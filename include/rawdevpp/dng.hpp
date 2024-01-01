#ifndef _RAWDEVPP_DNG_HPP_
#define _RAWDEVPP_DNG_HPP_

#include "color.hpp"
#include "tiff.hpp"

namespace rawdevpp
{
    namespace Decoder
    {
        struct DNG
        {
            struct ParsedIFD
            {
                template <typename T>
                static std::optional<T> extractInt(const std::optional<TIFF::DE> &property, std::istream &file, bool byteswap)
                {
                    if (!property.has_value())
                        return {};
                    return property.value().value<std::vector<T>>(file, byteswap)[0];
                }
                static std::optional<Eigen::VectorXd> extractVector(const std::optional<TIFF::DE> &property, size_t N, std::istream &file, bool byteswap)
                {
                    if (!property.has_value())
                        return {};
                    auto raw = property.value().value<std::vector<std::pair<uint32_t, uint32_t>>>(file, byteswap);
                    std::vector<double> data;
                    std::transform(raw.begin(), raw.end(), std::back_inserter(data), [](auto value)
                                   { return value.first / (double)value.second; });
                    Eigen::VectorXd ret = Eigen::Map<Eigen::VectorXd>(data.data(), N, 1);
                    return ret;
                }
                static std::optional<Eigen::MatrixXd> extractMatrix(const std::optional<TIFF::DE> &property, size_t R, size_t C, std::istream &file, bool byteswap)
                {
                    if (!property.has_value())
                        return {};
                    auto raw = property.value().value<std::vector<std::pair<int32_t, int32_t>>>(file, byteswap);
                    std::vector<double> data;
                    std::transform(raw.begin(), raw.end(), std::back_inserter(data), [](auto value)
                                   { return value.first / (double)value.second; });
                    Eigen::MatrixXd ret = Eigen::Map<Eigen::MatrixXd>(data.data(), R, C);
                    return ret;
                }
                template <typename Raw>
                static std::optional<double> extractLightSourceTemperature(const std::optional<TIFF::DE> &property, std::istream &file, bool byteswap)
                {
                    auto lightSource = extractInt<uint16_t>(property, file, byteswap);
                    if (!lightSource.has_value())
                        return {};
                    if (lightSource.value() & 0x8000)
                        return double(lightSource.value() & 0x7fff);

                    switch (lightSource.value())
                    {
                    case 1:
                    case 4:
                    case 9:
                    case 18:
                    case 20:
                        return 5500.0;

                    case 2:
                    case 14:
                        return 4200.0;

                    case 3:
                    case 17:
                        return 2850.0;

                    case 10:
                    case 19:
                    case 21:
                        return 6500.0;

                    case 11:
                    case 22:
                        return 7500.0;

                    case 12:
                        return 6400.0;

                    case 13:
                    case 23:
                        return 5000.0;

                    case 15:
                        return 3450.0;

                    case 24:
                        return 3200.0;

                    default:
                        return 0.0;
                    }
                }

                TIFF::IFD base;
                size_t colorPlanes;
                // TIFF-EP
                std::optional<TIFF::DE> cfaRepeatPatternDim;
                std::optional<TIFF::DE> cfaPattern;

                // DNG
                std::optional<TIFF::DE> dngVersion;
                std::optional<TIFF::DE> dngBackwardVersion;
                std::optional<TIFF::DE> uniqueCameraModel;
                std::optional<TIFF::DE> localizedCameraModel;
                std::optional<TIFF::DE> cfaPlaneColor;
                std::optional<TIFF::DE> cfaLayout;
                std::optional<TIFF::DE> linearizationTable;
                std::optional<TIFF::DE> blackLevelRepeatDim;
                std::optional<TIFF::DE> blackLevel;
                std::optional<TIFF::DE> blackLevelDeltaH;
                std::optional<TIFF::DE> blackLevelDeltaV;
                std::optional<TIFF::DE> whiteLevel;
                std::optional<TIFF::DE> defaultScale;
                std::optional<TIFF::DE> bestQualityScale;
                std::optional<TIFF::DE> defaultCropOrigin;
                std::optional<TIFF::DE> defaultCropSize;
                std::optional<double> calibrationIlluminant1;
                std::optional<double> calibrationIlluminant2;
                std::optional<Eigen::MatrixXd> colorMatrix1;
                std::optional<Eigen::MatrixXd> colorMatrix2;
                std::optional<Eigen::MatrixXd> cameraCalibration1;
                std::optional<Eigen::MatrixXd> cameraCalibration2;
                std::optional<Eigen::MatrixXd> reductionMatrix1;
                std::optional<Eigen::MatrixXd> reductionMatrix2;
                std::optional<Eigen::VectorXd> analogBalance;
                std::optional<Eigen::VectorXd> asShotNeutral;
                std::optional<TIFF::DE> asShotWhiteXy;
                std::optional<TIFF::DE> baselineExposure;
                std::optional<TIFF::DE> baselineNoise;
                std::optional<TIFF::DE> baselineSharpness;
                std::optional<TIFF::DE> bayerGreenSplit;
                std::optional<TIFF::DE> linearResponseLimit;
                std::optional<TIFF::DE> cameraSerialNumber;
                std::optional<TIFF::DE> lensInfo;
                std::optional<TIFF::DE> chromaBlurRadius;
                std::optional<TIFF::DE> antiAliasStrength;
                std::optional<TIFF::DE> shadowScale;
                std::optional<TIFF::DE> dngPrivateData;
                std::optional<TIFF::DE> makerNoteSafety;
                std::optional<TIFF::DE> rawDataUniqueID;
                std::optional<TIFF::DE> originalRawFileName;
                std::optional<TIFF::DE> originalRawFileData;
                std::optional<TIFF::DE> activeArea;
                std::optional<TIFF::DE> maskedAreas;
                std::optional<TIFF::DE> asShotICCProfile;
                std::optional<TIFF::DE> asShotPreProfileMatrix;
                std::optional<TIFF::DE> currentICCProfile;
                std::optional<TIFF::DE> currentPreProfileMatrix;
                std::optional<TIFF::DE> colorimetricReference;
                std::optional<TIFF::DE> cameraCalibrationSignature;
                std::optional<TIFF::DE> profileCalibrationSignature;
                std::optional<TIFF::DE> extraCameraProfiles;
                std::optional<TIFF::DE> asShotProfileName;
                std::optional<TIFF::DE> noiseReductionApplied;
                std::optional<TIFF::DE> profileName;
                std::optional<TIFF::DE> profileHueSatMapDims;
                std::optional<TIFF::DE> profileHueSatMapData1;
                std::optional<TIFF::DE> profileHueSatMapData2;
                std::optional<TIFF::DE> profileToneCurve;
                std::optional<TIFF::DE> profileEmbedPolicy;
                std::optional<TIFF::DE> profileCopyright;
                std::optional<Eigen::MatrixXd> forwardMatrix1;
                std::optional<Eigen::MatrixXd> forwardMatrix2;
                std::optional<TIFF::DE> previewApplicationName;
                std::optional<TIFF::DE> previewApplicationVersion;
                std::optional<TIFF::DE> previewSettingsName;
                std::optional<TIFF::DE> previewSettingsDigest;
                std::optional<TIFF::DE> previewColorSpace;
                std::optional<TIFF::DE> previewDateTime;
                std::optional<TIFF::DE> rawImageDigest;
                std::optional<TIFF::DE> originalRawFileDigest;
                std::optional<TIFF::DE> subTileBlockSize;
                std::optional<TIFF::DE> rowInterleaveFactor;
                std::optional<TIFF::DE> profileLookTableDims;
                std::optional<TIFF::DE> profileLookTableData;
                std::optional<TIFF::DE> opcodeList1;
                std::optional<TIFF::DE> opcodeList2;
                std::optional<TIFF::DE> opcodeList3;
                std::optional<TIFF::DE> noiseProfile;

                static ParsedIFD parse(const TIFF::IFD &base, std::istream &file, bool byteswap)
                {
                    auto getTagValues = [](const TIFF::IFD &ifd, uint16_t tag) -> std::optional<TIFF::DE>
                    {
                        if (ifd.properties.find(tag) != ifd.properties.end())
                            return ifd.properties.at(tag);
                        return {};
                    };
                    ParsedIFD ret;
                    ret.base = base;

                    size_t colorPlanes = base.getImageInfo(file, byteswap).samplePerPixel;
                    ret.colorPlanes = colorPlanes;

                    // TIFF-EP
                    ret.cfaRepeatPatternDim = getTagValues(base, 0x828d);
                    ret.cfaPattern = getTagValues(base, 0x828e);

                    // DNG
                    ret.dngVersion = getTagValues(base, 0xc612);
                    ret.dngBackwardVersion = getTagValues(base, 0xc613);
                    ret.uniqueCameraModel = getTagValues(base, 0xc614);
                    ret.localizedCameraModel = getTagValues(base, 0xc615);
                    ret.cfaPlaneColor = getTagValues(base, 0xc616);
                    ret.cfaLayout = getTagValues(base, 0xc617);
                    ret.linearizationTable = getTagValues(base, 0xc618);
                    ret.blackLevelRepeatDim = getTagValues(base, 0xc619);
                    ret.blackLevel = getTagValues(base, 0xc61a);
                    ret.blackLevelDeltaH = getTagValues(base, 0xc61b);
                    ret.blackLevelDeltaV = getTagValues(base, 0xc61c);
                    ret.whiteLevel = getTagValues(base, 0xc61d);
                    ret.defaultScale = getTagValues(base, 0xc61e);
                    ret.bestQualityScale = getTagValues(base, 0xc65c);
                    ret.defaultCropOrigin = getTagValues(base, 0xc61f);
                    ret.defaultCropSize = getTagValues(base, 0xc620);
                    ret.calibrationIlluminant1 = extractLightSourceTemperature<uint16_t>(getTagValues(base, 0xc65a), file, byteswap);
                    ret.calibrationIlluminant2 = extractLightSourceTemperature<uint16_t>(getTagValues(base, 0xc65b), file, byteswap);
                    ret.colorMatrix1 = extractMatrix(getTagValues(base, 0xC621), colorPlanes, 3, file, byteswap);
                    ret.colorMatrix2 = extractMatrix(getTagValues(base, 0xC622), colorPlanes, 3, file, byteswap);
                    ret.cameraCalibration1 = extractMatrix(getTagValues(base, 0xc623), colorPlanes, colorPlanes, file, byteswap);
                    ret.cameraCalibration2 = extractMatrix(getTagValues(base, 0xc624), colorPlanes, colorPlanes, file, byteswap);
                    ret.reductionMatrix1 = extractMatrix(getTagValues(base, 0xc625), 3, colorPlanes, file, byteswap);
                    ret.reductionMatrix2 = extractMatrix(getTagValues(base, 0xc626), 3, colorPlanes, file, byteswap);
                    ret.analogBalance = extractVector(getTagValues(base, 0xc627), colorPlanes, file, byteswap);
                    ret.asShotNeutral = extractVector(getTagValues(base, 0xc628), colorPlanes, file, byteswap);
                    ret.asShotWhiteXy = getTagValues(base, 0xc629);
                    ret.baselineExposure = getTagValues(base, 0xc62a);
                    ret.baselineNoise = getTagValues(base, 0xc62b);
                    ret.baselineSharpness = getTagValues(base, 0xc62c);
                    ret.bayerGreenSplit = getTagValues(base, 0xc62d);
                    ret.linearResponseLimit = getTagValues(base, 0xc62e);
                    ret.cameraSerialNumber = getTagValues(base, 0xc62f);
                    ret.lensInfo = getTagValues(base, 0xc630);
                    ret.chromaBlurRadius = getTagValues(base, 0xc631);
                    ret.antiAliasStrength = getTagValues(base, 0xc632);
                    ret.shadowScale = getTagValues(base, 0xc633);
                    ret.dngPrivateData = getTagValues(base, 0xc634);
                    ret.makerNoteSafety = getTagValues(base, 0xc635);
                    ret.rawDataUniqueID = getTagValues(base, 0xc65d);
                    ret.originalRawFileName = getTagValues(base, 0xc65d);
                    ret.originalRawFileData = getTagValues(base, 0xc68c);
                    ret.activeArea = getTagValues(base, 0xc68d);
                    ret.maskedAreas = getTagValues(base, 0xc68e);
                    ret.asShotICCProfile = getTagValues(base, 0xc68f);
                    ret.asShotPreProfileMatrix = getTagValues(base, 0xc690);
                    ret.currentICCProfile = getTagValues(base, 0xc691);
                    ret.currentPreProfileMatrix = getTagValues(base, 0xc692);
                    ret.colorimetricReference = getTagValues(base, 0xc6bf);
                    ret.cameraCalibrationSignature = getTagValues(base, 0xc6f3);
                    ret.profileCalibrationSignature = getTagValues(base, 0xc6f4);
                    ret.extraCameraProfiles = getTagValues(base, 0xc6f5);
                    ret.asShotProfileName = getTagValues(base, 0xc6f6);
                    ret.noiseReductionApplied = getTagValues(base, 0xc6f7);
                    ret.profileName = getTagValues(base, 0xc6f8);
                    ret.profileHueSatMapDims = getTagValues(base, 0xc6f9);
                    ret.profileHueSatMapData1 = getTagValues(base, 0xc6fa);
                    ret.profileHueSatMapData2 = getTagValues(base, 0xc6fb);
                    ret.profileToneCurve = getTagValues(base, 0xc6fc);
                    ret.profileEmbedPolicy = getTagValues(base, 0xc6fd);
                    ret.profileCopyright = getTagValues(base, 0xc6fe);
                    ret.forwardMatrix1 = extractMatrix(getTagValues(base, 0xc714), 3, colorPlanes, file, byteswap);
                    ret.forwardMatrix2 = extractMatrix(getTagValues(base, 0xc715), 3, colorPlanes, file, byteswap);
                    ret.previewApplicationName = getTagValues(base, 0xc716);
                    ret.previewApplicationVersion = getTagValues(base, 0xc717);
                    ret.previewSettingsName = getTagValues(base, 0xc718);
                    ret.previewSettingsDigest = getTagValues(base, 0xc719);
                    ret.previewColorSpace = getTagValues(base, 0xc71a);
                    ret.previewDateTime = getTagValues(base, 0xc71b);
                    ret.rawImageDigest = getTagValues(base, 0xc71c);
                    ret.originalRawFileDigest = getTagValues(base, 0xc71d);
                    ret.subTileBlockSize = getTagValues(base, 0xc71e);
                    ret.rowInterleaveFactor = getTagValues(base, 0xc71f);
                    ret.profileLookTableDims = getTagValues(base, 0xc725);
                    ret.profileLookTableData = getTagValues(base, 0xc726);
                    ret.opcodeList1 = getTagValues(base, 0xc740);
                    ret.opcodeList2 = getTagValues(base, 0xc741);
                    ret.opcodeList3 = getTagValues(base, 0xc74e);
                    ret.noiseProfile = getTagValues(base, 0xc761);

                    return ret;
                }

                Eigen::MatrixXd matrixCamera2ProPhotoRGB(const Context &ctx) const;
            };

            TIFF tiff;
            std::vector<ParsedIFD> images;

            static DNG parse(std::istream &file)
            {
                DNG ret;
                ret.tiff = TIFF::parse(file);

                for (const auto &it : ret.tiff.images)
                    ret.images.push_back(ParsedIFD::parse(it, file, ret.tiff.byteswap));
                return ret;
            }
        };
    }

    namespace Color
    {
        using Camera = Eigen::Array3Xd;

        inline Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> matrixInterpolation(const std::optional<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> &matrix1, const std::optional<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> &matrix2, double value1, double value2, double value, size_t R = 1, size_t C = 1)
        {
            if (!matrix1.has_value() && !matrix2.has_value())
                return Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>::Identity(R, C);
            else if (matrix1.has_value() && !matrix2.has_value())
                return matrix1.value();
            else if (value < value1)
                return matrix1.value();
            else if (value > value2)
                return matrix2.value();

            double f = ((1.0 / value) - (1.0 / value2)) / ((1.0 / value1) - (1.0 / value2));
            double f1 = 1.0 - f;
            Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> ret;
            ret = (matrix1.value().array() * f + matrix2.value().array() * f1).matrix();
            return ret;
        }

        inline Eigen::MatrixX3d matrixXYZ2Camera(const Context &ctx, const Decoder::DNG::ParsedIFD &ifd, const XY &whiteBalanceXY)
        {
            auto whiteBalanceTemperature = XY2Temperature(ctx, whiteBalanceXY);
            double calibrationIlluminant1 = ifd.calibrationIlluminant1.value_or(0.0);
            double calibrationIlluminant2 = ifd.calibrationIlluminant2.value_or(0.0);
            auto colorMatrix = matrixInterpolation(ifd.colorMatrix1, ifd.colorMatrix2, calibrationIlluminant1, calibrationIlluminant2, whiteBalanceTemperature(0, 0));
            // let reductionMatrix = DngMath.matrixInterpolation(tags.reductionMatrix1, tags.reductionMatrix2, tags.calibrationIlluminant1, tags.calibrationIlluminant2, whiteBalanceTemperature.temperature)
            auto cameraCalibration = matrixInterpolation(ifd.cameraCalibration1, ifd.cameraCalibration2, calibrationIlluminant1, calibrationIlluminant2, whiteBalanceTemperature(0, 0), ifd.colorPlanes, ifd.colorPlanes);
            Eigen::MatrixXd analogBalance = Eigen::MatrixXd::Identity(ifd.colorPlanes, ifd.colorPlanes);
            if (ifd.analogBalance.has_value())
                analogBalance = ifd.analogBalance.value().asDiagonal();

            return analogBalance * cameraCalibration * colorMatrix;
        }

        inline Eigen::Matrix3Xd matrixCamera2D50(const Context &ctx, const Decoder::DNG::ParsedIFD &ifd, const XY &whiteBalanceXY)
        {
            XYZ whiteBalanceXYZ = XY2XYZ(whiteBalanceXY);
            auto matXYZ2Camera = matrixXYZ2Camera(ctx, ifd, whiteBalanceXY);
            auto matCamera2XYZ = matXYZ2Camera.inverse();
            auto chromaticAdaptation = whitePointXYZConvertMatrix(whiteBalanceXYZ, D50());
            auto ret = chromaticAdaptation * matCamera2XYZ;
            return ret;
        }

        inline XY cameraNeutralWhiteBalance(const Context &ctx, const Decoder::DNG::ParsedIFD &ifd)
        {
            Eigen::VectorXd neutralWhiteBalance = ifd.asShotNeutral.value_or(Eigen::VectorXd::Ones(ifd.colorPlanes));
            XY last = XYZ2XY(D50());
            double d = 1.0;
            do
            {
                XY current = XYZ2XY(matrixXYZ2Camera(ctx, ifd, last).inverse() * neutralWhiteBalance);
                d = std::abs(last(0, 0) - current(0, 0)) + std::abs(last(1, 0) - current(1, 0));
            } while (d > 0.0000001);

            return last;
        }
    }

    namespace Decoder
    {
        Eigen::MatrixXd DNG::ParsedIFD::matrixCamera2ProPhotoRGB(const Context &ctx) const
        {
            Color::XY whiteBalance = Color::cameraNeutralWhiteBalance(ctx, *this);
            return Color::matrixXYZ2ProPhotoRGB() * Color::matrixCamera2D50(ctx, *this, whiteBalance);
        }
    }
}

#endif
