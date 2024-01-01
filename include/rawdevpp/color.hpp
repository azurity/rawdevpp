#ifndef _RAWDEVPP_COLOR_HPP_
#define _RAWDEVPP_COLOR_HPP_

#include <array>
#include <Eigen/Eigen>
#include "context.hpp"

namespace rawdevpp
{
    namespace Color
    {
        using Channel = Eigen::Array<double, 1, Eigen::Dynamic>;
        using Index = Eigen::Array<int, 1, Eigen::Dynamic>;
        using RGB = Eigen::Array3Xd;         // {R,G,B}
        using HSV = Eigen::Array3Xd;         // {H,S,V}
        using XYZ = Eigen::Array3Xd;         // {X,Y,Z}
        using XY = Eigen::Array2Xd;          // {X,Y}
        using Temperature = Eigen::Array2Xd; // {temperature,tint}
        using WhitePoint = Eigen::Array3Xd;

        enum class ColorSpace
        {
            XYZ,
            XY,
            SRGB,
            ProPhotoRGB,
            HSV,
        };

        inline HSV RGB2HSV(const RGB &rgb)
        {
            auto max = rgb.colwise().maxCoeff();
            auto min = rgb.colwise().minCoeff();
            HSV ret = HSV::Zero(3, rgb.cols());

            auto m = max - min;
            auto hR = (rgb.row(1) - rgb.row(2)) / m;
            auto hG = (rgb.row(2) - rgb.row(0)) / m + 2;
            auto hB = (rgb.row(0) - rgb.row(1)) / m + 4;
            ret.row(0) = (max == rgb.row(0)).select(hR, ret.row(0));
            ret.row(0) = (max == rgb.row(1)).select(hG, ret.row(0));
            ret.row(0) = (max == rgb.row(2)).select(hB, ret.row(0));
            ret.row(0) += 6;
            ret.row(0) -= Eigen::floor((ret.row(0) / 6)).eval() * 6;
            ret.row(0) *= 60;
            ret.row(1) = m / max;
            ret.row(2) = max;

            return ret;
        }

        inline RGB HSV2RGB(const HSV &hsv)
        {
            Channel h = hsv.row(0).cwiseMax(0).cwiseMin(360);
            auto s = hsv.row(1).cwiseMax(0).cwiseMin(1);
            auto v = hsv.row(2).cwiseMax(0).cwiseMin(1);

            RGB ret = RGB::Zero(3, hsv.cols());
            h = (h - Eigen::floor(h / 360) * 360) / 60;
            auto f = h - Eigen::floor(h);
            auto i = Eigen::floor(h).cast<int>();
            auto p = v * (1 - s);
            auto q = v * (1 - Channel(s * f));
            auto t = v * (1 - Channel(s * (1 - f)));
            // i == 0
            ret.row(0) = (i == 0).select(v, ret.row(0));
            ret.row(1) = (i == 0).select(t, ret.row(1));
            ret.row(2) = (i == 0).select(p, ret.row(2));
            // i == 1
            ret.row(0) = (i == 1).select(q, ret.row(0));
            ret.row(1) = (i == 1).select(v, ret.row(1));
            ret.row(2) = (i == 1).select(p, ret.row(2));
            // i == 2
            ret.row(0) = (i == 2).select(p, ret.row(0));
            ret.row(1) = (i == 2).select(v, ret.row(1));
            ret.row(2) = (i == 2).select(t, ret.row(2));
            // i == 3
            ret.row(0) = (i == 3).select(p, ret.row(0));
            ret.row(1) = (i == 3).select(q, ret.row(1));
            ret.row(2) = (i == 3).select(v, ret.row(2));
            // i == 4
            ret.row(0) = (i == 4).select(t, ret.row(0));
            ret.row(1) = (i == 4).select(p, ret.row(1));
            ret.row(2) = (i == 4).select(v, ret.row(2));
            // i == 5
            ret.row(0) = (i == 5).select(v, ret.row(0));
            ret.row(1) = (i == 5).select(p, ret.row(1));
            ret.row(2) = (i == 5).select(q, ret.row(2));
            // s == 0
            ret.row(0) = (s == 0).select(v, ret.row(0));
            ret.row(1) = (s == 0).select(v, ret.row(1));
            ret.row(2) = (s == 0).select(v, ret.row(2));
            //
            return ret;
        }

        inline Temperature XY2Temperature(const Context &ctx, const XY &xy)
        {
            auto us = 2.0 * xy.row(0).array() / (1.5 - xy.row(0).array() + 6.0 * xy.row(1).array());
            auto vs = 3.0 * xy.row(1).array() / (1.5 - xy.row(0).array() + 6.0 * xy.row(1).array());

            Eigen::Array<double, 1, Eigen::Dynamic> di = Eigen::Array<double, 1, Eigen::Dynamic>::Zero(1, xy.cols());
            Eigen::Array<double, 1, Eigen::Dynamic> dj = Eigen::Array<double, 1, Eigen::Dynamic>::Zero(1, xy.cols());
            Index index = Eigen::Array<int, 1, Eigen::Dynamic>::Zero(1, xy.cols());

            for (size_t i = 0; i < 31; i++)
            {
                auto diNew = (vs - ctx.ruvtTable(Context::V, i)) - ctx.ruvtTable(Context::T, i) * (us - ctx.ruvtTable(Context::U, i));

                if (i > 0)
                {
                    index = (di < 0).select(index, i);
                    di = (di < 0).select(di, diNew);
                }
                dj = (di < 0).select(dj, di);
            }
            di /= Eigen::sqrt(1.0 + ctx.ruvtTable(index, Context::T).pow(2));
            dj = Eigen::sqrt(1.0 + ctx.ruvtTable(index - 1, Context::T).pow(2));

            auto f = dj / (dj - di);

            Temperature ret = Temperature::Zero(2, xy.cols());
            ret.row(0) = 1000000.0 / ((ctx.ruvtTable(index, Context::R) - ctx.ruvtTable(index - 1, Context::R)) * f + ctx.ruvtTable(index - 1, Context::R));

            auto ud = us - ((ctx.ruvtTable(index, Context::U) - ctx.ruvtTable(index - 1, Context::U)) * f + ctx.ruvtTable(index - 1, Context::U));
            auto vd = vs - ((ctx.ruvtTable(index, Context::V) - ctx.ruvtTable(index - 1, Context::V)) * f + ctx.ruvtTable(index - 1, Context::V));

            auto tli = Eigen::sqrt(1.0 + ctx.ruvtTable(index, Context::T).pow(2));
            auto tui = 1.0 / tli;
            auto tvi = ctx.ruvtTable(index, Context::T) / tli;

            auto tlj = Eigen::sqrt(1.0 + ctx.ruvtTable(index - 1, Context::T).pow(2));
            auto tuj = 1.0 / tlj;
            auto tvj = ctx.ruvtTable(index - 1, Context::T) / tlj;

            Channel tu = (tui - tuj) * f + tuj;
            Channel tv = (tvi - tvj) * f + tvj;
            Channel tl = Eigen::sqrt(tu.pow(2) + tv.pow(2));

            tu /= tl;
            tv /= tl;

            ret.row(1) = (ud * tu + vd * tv) * -3000.0;

            return ret;
        }

        inline XY Temperature2Y(const Context &ctx, const Temperature &t)
        {
            Channel r = 1000000.0 / t.row(0);
            Index index = Index::Ones(1, t.cols());
            for (size_t i = 0; i < 31; i++)
            {
                index = (r >= ctx.ruvtTable(i, r)).select(i, index);
            }

            auto f = (ctx.ruvtTable(index, Context::R) - r) / (ctx.ruvtTable(index, Context::R) - ctx.ruvtTable(index - 1, Context::R));

            Channel us = ctx.ruvtTable(index - 1, Context::U) - ctx.ruvtTable(index, Context::U) * f + ctx.ruvtTable(index, Context::U);
            Channel vs = ctx.ruvtTable(index - 1, Context::V) - ctx.ruvtTable(index, Context::V) * f + ctx.ruvtTable(index, Context::V);

            auto tli = Eigen::sqrt(1.0 + ctx.ruvtTable(index, Context::T).pow(2));
            auto tui = 1.0 / tli;
            auto tvi = ctx.ruvtTable(index, Context::T) / tli;

            auto tlj = Eigen::sqrt(1.0 + ctx.ruvtTable(index - 1, Context::T).pow(2));
            auto tuj = 1.0 / tlj;
            auto tvj = ctx.ruvtTable(index - 1, Context::T) / tlj;

            Channel tu = (tuj - tui) * f + tui;
            Channel tv = (tvj - tvi) * f + tvi;
            Channel tl = Eigen::sqrt(tu.pow(2) + tv.pow(2));

            tu /= tl;
            tv /= tl;

            us += tu * t.row(1) / -3000.0;
            vs += tv * t.row(1) / -3000.0;

            XY ret = XY::Zero(2, t.cols());
            ret.row(0) = 1.5 * us / (us - 4.0 * vs + 2.0);
            ret.row(1) = vs / (us - 4.0 * vs + 2.0);
            return ret;
        }

        inline XYZ XY2XYZ(const XY &xy)
        {
            XYZ ret = XYZ::Ones(3, xy.cols());
            ret.row(0) = xy.row(0) / xy.row(1);
            ret.row(2) = (1.0 - xy.row(0) - xy.row(1)) / xy.row(1);
            return ret;
        }

        inline XY XYZ2XY(const XYZ &xyz)
        {
            auto s = xyz.colwise().sum();
            XY ret = xyz.topRows(2);
            ret.row(0) /= s;
            ret.row(1) /= s;
            return ret;
        }

        inline Eigen::Matrix3d whitePointXYZConvertMatrix(const WhitePoint &source, const WhitePoint target)
        {
            Eigen::Matrix3d bradfordMatrix;
            bradfordMatrix << 0.8951, 0.2664, -0.1614, -0.7502, 1.7135, 0.0367, 0.0389, -0.0685, 1.0296;

            Eigen::Matrix3d ADT = ((bradfordMatrix * target.matrix().col(0)).array() / (bradfordMatrix * source.matrix().col(0)).array()).matrix().asDiagonal();
            return bradfordMatrix.inverse() * ADT * bradfordMatrix;
        }

        inline WhitePoint D50()
        {
            XY wb(2, 1);
            wb(0, 0) = 0.34567;
            wb(1, 0) = 0.35850;
            return XY2XYZ(wb);
        }

        inline WhitePoint D65()
        {
            XY wb(2, 1);
            wb(0, 0) = 0.31271;
            wb(1, 0) = 0.32902;
            return XY2XYZ(wb);
        }

        inline Eigen::Matrix3d matrixProPhotoRGB2XYZ()
        {
            Eigen::Matrix3d ret;
            ret << 0.797675, 0.135192, 0.0313534, 0.288040, 0.711874, 0.000086, 0.0, 0.0, 0.825210;
            return ret;
        }

        inline Eigen::Matrix3d matrixXYZ2ProPhotoRGB()
        {
            Eigen::Matrix3d ret;
            ret << 1.34594, -0.255608, -0.0511118, -0.544599, 1.50817, 0.0205351, 0.0, 0.0, 1.21181;
            return ret;
        }

        inline Eigen::Matrix3d matrixSRGB2XYZ()
        {
            Eigen::Matrix3d ret;
            ret << 0.412424, 0.357579, 0.180464, 0.212656, 0.715158, 0.0721856, 0.0193324, 0.119193, 0.950444;
            return ret;
        }

        // D65
        inline Eigen::Matrix3d matrixXYZ2SRGB()
        {
            Eigen::Matrix3d ret;
            ret << 3.24071, -1.53726, -0.498571, -0.969258, 1.87599, 0.0415557, 0.0556352, -0.203996, 1.05707;
            return ret;
        }
    }
}

#endif
