#ifndef _RAWDEVPP_TIFF_HPP_
#define _RAWDEVPP_TIFF_HPP_

#include <algorithm>
#include <bit>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <string>
#include <vector>
#include <Eigen/Eigen>

namespace rawdevpp
{
    namespace Decoder
    {
        struct TIFF
        {
            using Tag = uint16_t;

            static constexpr const Tag TAG_NEW_SUBFILE_TYPE = 0xfe;            // LONG
            static constexpr const Tag TAG_IMAGE_WIDTH = 0x100;                // SHORT or LONG
            static constexpr const Tag TAG_IMAGE_LENGTH = 0x101;               // SHORT or LONG
            static constexpr const Tag TAG_BITS_PER_SAMPLE = 0x102;            // SHORT
            static constexpr const Tag TAG_COMPRESSION = 0x0103;               // SHORT
            static constexpr const Tag TAG_PHOTOMETRIC_INTERPRETATION = 0x106; // SHORT
            static constexpr const Tag TAG_FILL_ORDER = 0x10A;                 // SHORT
            static constexpr const Tag TAG_STRIP_OFFSET = 0x111;               // SHORT or LONG
            static constexpr const Tag TAG_SAMPLE_PER_PIXEL = 0x115;           // SHORT
            static constexpr const Tag TAG_ROWS_PER_STRIP = 0x116;             // SHORT or LONG
            static constexpr const Tag TAG_STRIP_BYTE_COUNTS = 0x117;          // SHORT or LONG
            static constexpr const Tag TAG_PLANAR_CONFIGURATION = 0x11c;       // SHORT
            static constexpr const Tag TAG_TILE_WIDTH = 0x142;                 // SHORT or LONG
            static constexpr const Tag TAG_TILE_LENGTH = 0x143;                // SHORT or LONG
            static constexpr const Tag TAG_TILE_OFFSETS = 0x144;               // LONG
            static constexpr const Tag TAG_TILE_BYTE_COUNTS = 0x145;           // SHORT or LONG
            static constexpr const Tag TAG_SUB_IFDS = 0x14a;

            template <Tag T>
            struct TagType
            {
            };

            enum class DEDataType : uint16_t
            {
                BYTE = 1,
                ASCII = 2,
                SHORT = 3,
                LONG = 4,
                RATIONAL = 5,
                SBYTE = 6,
                UNDEFINED = 7,
                SSHORT = 8,
                SLONG = 9,
                SRATIONAL = 10,
                FLOAT = 11,
                DOUBLE = 12,
            };

            static inline size_t DEDataTypeSize(DEDataType type)
            {
                switch (type)
                {
                case DEDataType::BYTE:
                case DEDataType::ASCII:
                case DEDataType::SBYTE:
                case DEDataType::UNDEFINED:
                    return 1;
                case DEDataType::SHORT:
                case DEDataType::SSHORT:
                    return 2;
                case DEDataType::LONG:
                case DEDataType::SLONG:
                case DEDataType::FLOAT:
                    return 4;
                case DEDataType::RATIONAL:
                case DEDataType::SRATIONAL:
                case DEDataType::DOUBLE:
                    return 8;
                default:
                    return 1;
                }
            }

            struct IFH
            {
                uint16_t order;
                uint16_t version;
                uint32_t offset;
            };

            struct IFD;

            struct DE
            {
                uint16_t tag;
                uint16_t type;
                uint32_t length;
                uint32_t valueOffset;

                static DE parse(std::istream &file, const bool byteswap)
                {
                    DE ret;
                    file.read((char *)&ret, sizeof(DE));
                    uint32_t offset = file.tellg();
                    offset -= 4;
                    if (byteswap)
                    {
                        ret.tag = std::byteswap(ret.tag);
                        ret.type = std::byteswap(ret.type);
                        ret.length = std::byteswap(ret.length);
                        ret.valueOffset = std::byteswap(ret.valueOffset);
                    }
                    if (DEDataTypeSize(DEDataType(ret.type)) * ret.length <= 4)
                    {
                        ret.valueOffset = offset;
                    }
                    return ret;
                }

                template <typename T = void>
                T value(std::istream &file, bool byteswap) const {}

                std::vector<IFD> valueIFD(std::istream &file, bool byteswap) const
                {
                    std::vector<IFD> ret;
                    uint32_t offset = valueOffset;
                    while (offset != 0)
                    {
                        file.seekg(offset, std::ios::beg);
                        auto entryList = IFD::parse(file, byteswap);
                        offset = entryList.nextOffset;
                        ret.push_back(entryList);
                    }
                    return ret;
                }
            };

            struct ImageInfo
            {
                size_t planarConfig;
                size_t compressionType;
                size_t width;
                size_t height;
                size_t bitsPerSample;
                size_t samplePerPixel;
                size_t fillOrder;

                size_t rowsPerStrip;
                std::vector<size_t> stripOffset;
                std::vector<size_t> stripByteCounts;

                size_t tileWidth;
                size_t tileHeight;
                std::vector<size_t> tileOffset;
                std::vector<size_t> tileByteCounts;
            };

            struct IFD
            {
                uint16_t size;
                std::map<uint16_t, DE> properties;
                uint32_t nextOffset;

                static IFD parse(std::istream &file, const bool byteswap)
                {
                    IFD ret;
                    file.read((char *)&ret.size, sizeof(uint16_t));
                    if (byteswap)
                        ret.size = std::byteswap(ret.size);
                    for (size_t i = 0; i < ret.size; i++)
                    {
                        auto property = DE::parse(file, byteswap);
                        ret.properties[property.tag] = property;
                    }
                    file.read((char *)&ret.nextOffset, sizeof(uint32_t));
                    if (byteswap)
                        ret.nextOffset = std::byteswap(ret.nextOffset);
                    return ret;
                }
                std::vector<IFD> getSubDirectories(std::istream &file, bool byteswap) const
                {
                    auto aim = properties.find(TAG_SUB_IFDS);
                    if (aim == properties.end())
                        return {};
                    return aim->second.valueIFD(file, byteswap);
                }
                std::optional<IFD> getDirectoryBySubfileType(std::istream &file, bool byteswap, uint32_t value) const;
                ImageInfo getImageInfo(std::istream &file, bool byteswap) const;

                template <typename T>
                std::tuple<size_t, size_t, std::vector<T>> readImage(const ImageInfo &info, std::istream &file, bool byteswap) const;
            };

            bool byteswap;
            std::vector<IFD> images;

            static TIFF parse(std::istream &file)
            {
                TIFF ret;
                IFH fileHeader;
                file.read((char *)&fileHeader, sizeof(IFH));
                std::endian endian = std::endian::native;
                if (fileHeader.order == 0x4949)
                {
                    endian = std::endian::little;
                }
                else if (fileHeader.order == 0x4d4d)
                {
                    endian = std::endian::big;
                }
                ret.byteswap = endian != std::endian::native;
                if (ret.byteswap)
                    fileHeader.offset = std::byteswap(fileHeader.offset);

                uint32_t offset = fileHeader.offset;
                while (offset != 0)
                {
                    file.seekg(offset, std::ios::beg);
                    auto entryList = IFD::parse(file, ret.byteswap);
                    offset = entryList.nextOffset;
                    ret.images.push_back(entryList);
                }

                return ret;
            }
        };

        template <>
        std::string TIFF::DE::value<std::string>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<char> ret;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            ret.resize(length);
            file.read((char *)ret.data(), size);
            return std::string(ret.data());
        }
        template <>
        std::vector<uint8_t> TIFF::DE::value<std::vector<uint8_t>>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<uint8_t> ret;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            ret.resize(length);
            file.read((char *)ret.data(), size);
            return ret;
        }
        template <>
        std::vector<uint16_t> TIFF::DE::value<std::vector<uint16_t>>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<uint16_t> ret;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            ret.resize(length);
            file.read((char *)ret.data(), size);
            if (byteswap)
                for (size_t i = 0; i < length; i++)
                    ret[i] = std::byteswap(ret[i]);
            return ret;
        }
        template <>
        std::vector<uint32_t> TIFF::DE::value<std::vector<uint32_t>>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<uint32_t> ret;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            ret.resize(length);
            file.read((char *)ret.data(), size);
            if (byteswap)
                for (size_t i = 0; i < length; i++)
                    ret[i] = std::byteswap(ret[i]);
            return ret;
        }
        template <>
        std::vector<std::pair<uint32_t, uint32_t>> TIFF::DE::value<std::vector<std::pair<uint32_t, uint32_t>>>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<uint32_t> temp;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            temp.resize(length * 2);
            file.read((char *)temp.data(), size);
            if (byteswap)
                for (size_t i = 0; i < length * 2; i++)
                    temp[i] = std::byteswap(temp[i]);
            std::vector<std::pair<uint32_t, uint32_t>> ret;
            for (size_t i = 0; i < length; i++)
                ret.push_back(std::pair<uint32_t, uint32_t>{temp[i * 2], temp[i * 2 + 1]});
            return ret;
        }
        template <>
        std::vector<int8_t> TIFF::DE::value<std::vector<int8_t>>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<int8_t> ret;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            ret.resize(length);
            file.read((char *)ret.data(), size);
            return ret;
        }
        template <>
        std::vector<int16_t> TIFF::DE::value<std::vector<int16_t>>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<int16_t> ret;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            ret.resize(length);
            file.read((char *)ret.data(), size);
            if (byteswap)
                for (size_t i = 0; i < length; i++)
                    ret[i] = std::byteswap(ret[i]);
            return ret;
        }
        template <>
        std::vector<int32_t> TIFF::DE::value<std::vector<int32_t>>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<int32_t> ret;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            ret.resize(length);
            file.read((char *)ret.data(), size);
            if (byteswap)
                for (size_t i = 0; i < length; i++)
                    ret[i] = std::byteswap(ret[i]);
            return ret;
        }
        template <>
        std::vector<std::pair<int32_t, int32_t>> TIFF::DE::value<std::vector<std::pair<int32_t, int32_t>>>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<int32_t> temp;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            temp.resize(length * 2);
            file.read((char *)temp.data(), size);
            if (byteswap)
                for (size_t i = 0; i < length * 2; i++)
                    temp[i] = std::byteswap(temp[i]);
            std::vector<std::pair<int32_t, int32_t>> ret;
            for (size_t i = 0; i < length; i++)
                ret.push_back(std::pair<int32_t, int32_t>{temp[i * 2], temp[i * 2 + 1]});
            return ret;
        }
        template <>
        std::vector<float> TIFF::DE::value<std::vector<float>>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<float> ret;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            ret.resize(length);
            file.read((char *)ret.data(), size);
            if (byteswap)
                for (size_t i = 0; i < length; i++)
                {
                    uint32_t value = *reinterpret_cast<uint32_t *>(&ret[i]);
                    value = std::byteswap(value);
                    ret[i] = *reinterpret_cast<float *>(&value);
                }
            return ret;
        }
        template <>
        std::vector<double> TIFF::DE::value<std::vector<double>>(std::istream &file, bool byteswap) const
        {
            file.seekg(valueOffset);
            std::vector<double> ret;
            size_t size = DEDataTypeSize(DEDataType(type)) * length;
            ret.resize(length);
            file.read((char *)ret.data(), size);
            if (byteswap)
                for (size_t i = 0; i < length; i++)
                {
                    uint64_t value = *reinterpret_cast<uint32_t *>(&ret[i]);
                    value = std::byteswap(value);
                    ret[i] = *reinterpret_cast<double *>(&value);
                }
            return ret;
        }

        std::optional<TIFF::IFD> TIFF::IFD::getDirectoryBySubfileType(std::istream &file, bool byteswap, uint32_t value) const
        {
            if (properties.find(TAG_NEW_SUBFILE_TYPE) != properties.end() && properties.at(TAG_NEW_SUBFILE_TYPE).value<std::vector<uint32_t>>(file, byteswap)[0] == value)
                return *this;
            auto subs = getSubDirectories(file, byteswap);
            for (auto &it : subs)
            {
                auto ret = it.getDirectoryBySubfileType(file, byteswap, value);
                if (ret.has_value())
                    return ret;
            }
            return {};
        }

        inline TIFF::ImageInfo TIFF::IFD::getImageInfo(std::istream &file, bool byteswap) const
        {
            ImageInfo info;

            info.planarConfig = 1;
            if (properties.find(TAG_PLANAR_CONFIGURATION) != properties.end())
                info.planarConfig = properties.at(TAG_PLANAR_CONFIGURATION).value<std::vector<uint16_t>>(file, byteswap)[0];

            info.compressionType = 1;
            if (properties.find(TAG_COMPRESSION) != properties.end())
                info.compressionType = properties.at(TAG_COMPRESSION).value<std::vector<uint16_t>>(file, byteswap)[0];

            auto widthRaw = properties.at(TAG_IMAGE_WIDTH);
            if (widthRaw.type == (uint16_t)DEDataType::SHORT)
                info.width = widthRaw.value<std::vector<uint16_t>>(file, byteswap)[0];
            else
                info.width = widthRaw.value<std::vector<uint32_t>>(file, byteswap)[0];

            auto heightRaw = properties.at(TAG_IMAGE_LENGTH);
            if (heightRaw.type == (uint16_t)DEDataType::SHORT)
                info.height = heightRaw.value<std::vector<uint16_t>>(file, byteswap)[0];
            else
                info.height = heightRaw.value<std::vector<uint32_t>>(file, byteswap)[0];

            info.bitsPerSample = properties.at(TAG_BITS_PER_SAMPLE).value<std::vector<uint16_t>>(file, byteswap)[0];
            info.samplePerPixel = properties.at(TAG_SAMPLE_PER_PIXEL).value<std::vector<uint16_t>>(file, byteswap)[0];

            info.fillOrder = 1;
            if (properties.find(TAG_FILL_ORDER) != properties.end())
                info.fillOrder = properties.at(TAG_FILL_ORDER).value<std::vector<uint16_t>>(file, byteswap)[0];

            if (properties.find(TAG_ROWS_PER_STRIP) != properties.end())
            {
                auto rowsPerStripRaw = properties.at(TAG_ROWS_PER_STRIP);
                if (rowsPerStripRaw.type == (uint16_t)DEDataType::SHORT)
                    info.rowsPerStrip = rowsPerStripRaw.value<std::vector<uint16_t>>(file, byteswap)[0];
                else
                    info.rowsPerStrip = rowsPerStripRaw.value<std::vector<uint32_t>>(file, byteswap)[0];
            }
            if (properties.find(TAG_STRIP_OFFSET) != properties.end())
            {
                auto stripOffsetRaw = properties.at(TAG_STRIP_OFFSET);
                if (stripOffsetRaw.type == (uint16_t)DEDataType::SHORT)
                {
                    auto list = stripOffsetRaw.value<std::vector<uint16_t>>(file, byteswap);
                    std::copy(list.begin(), list.end(), std::back_inserter(info.stripOffset));
                }
                else
                {
                    auto list = stripOffsetRaw.value<std::vector<uint32_t>>(file, byteswap);
                    std::copy(list.begin(), list.end(), std::back_inserter(info.stripOffset));
                }
            }
            if (properties.find(TAG_STRIP_BYTE_COUNTS) != properties.end())
            {
                auto stripByteCountsRaw = properties.at(TAG_STRIP_BYTE_COUNTS);
                if (stripByteCountsRaw.type == (uint16_t)DEDataType::SHORT)
                {
                    auto list = stripByteCountsRaw.value<std::vector<uint16_t>>(file, byteswap);
                    std::copy(list.begin(), list.end(), std::back_inserter(info.stripByteCounts));
                }
                else
                {
                    auto list = stripByteCountsRaw.value<std::vector<uint32_t>>(file, byteswap);
                    std::copy(list.begin(), list.end(), std::back_inserter(info.stripByteCounts));
                }
            }

            if (properties.find(TAG_TILE_WIDTH) != properties.end())
            {
                auto tileWidthRaw = properties.at(TAG_TILE_WIDTH);
                if (tileWidthRaw.type == (uint16_t)DEDataType::SHORT)
                    info.tileWidth = tileWidthRaw.value<std::vector<uint16_t>>(file, byteswap)[0];
                else
                    info.tileWidth = tileWidthRaw.value<std::vector<uint32_t>>(file, byteswap)[0];
            }
            if (properties.find(TAG_TILE_LENGTH) != properties.end())
            {
                auto tileHeightRaw = properties.at(TAG_TILE_LENGTH);
                if (tileHeightRaw.type == (uint16_t)DEDataType::SHORT)
                    info.tileHeight = tileHeightRaw.value<std::vector<uint16_t>>(file, byteswap)[0];
                else
                    info.tileHeight = tileHeightRaw.value<std::vector<uint32_t>>(file, byteswap)[0];
            }
            if (properties.find(TAG_TILE_OFFSETS) != properties.end())
            {
                auto list = properties.at(TAG_TILE_OFFSETS).value<std::vector<uint32_t>>(file, byteswap);
                std::copy(list.begin(), list.end(), std::back_inserter(info.tileOffset));
            }
            if (properties.find(TAG_TILE_BYTE_COUNTS) != properties.end())
            {
                auto tileByteCountsRaw = properties.at(TAG_TILE_BYTE_COUNTS);
                if (tileByteCountsRaw.type == (uint16_t)DEDataType::SHORT)
                {
                    auto list = tileByteCountsRaw.value<std::vector<uint16_t>>(file, byteswap);
                    std::copy(list.begin(), list.end(), std::back_inserter(info.tileByteCounts));
                }
                else
                {
                    auto list = tileByteCountsRaw.value<std::vector<uint32_t>>(file, byteswap);
                    std::copy(list.begin(), list.end(), std::back_inserter(info.tileByteCounts));
                }
            }

            return info;
        }

        template <typename T>
        std::tuple<size_t, size_t, std::vector<T>> TIFF::IFD::readImage(const ImageInfo &info, std::istream &file, bool byteswap) const
        {
            if (info.compressionType != 1)
                return {};
            std::vector<T> ret;

            std::endian endian = info.fillOrder == 1 ? std::endian::big : std::endian::little;
            if (info.bitsPerSample != 8 && info.bitsPerSample != 16 && info.bitsPerSample != 32)
                endian = std::endian::big;

            std::function<std::vector<T>(const std::vector<uint8_t> &)> readUnit;

            const size_t sampleBits = info.bitsPerSample;
            const size_t unitLength = std::lcm(info.bitsPerSample, 8);
            const size_t unitCount = unitLength / info.bitsPerSample;
            const size_t unitBytes = unitLength / 8;

            std::vector<uint8_t> buffer;
            buffer.resize(unitBytes);

            if (endian == std::endian::little)
                readUnit = [unitCount, unitBytes](const std::vector<uint8_t> &buffer)
                {
                    std::vector<T> ret;
                    ret.reserve(unitCount);
                    size_t len = unitBytes / unitCount;
                    for (size_t i = 0; i < unitBytes; i += len)
                    {
                        uint32_t result = 0;
                        for (size_t j = 0; j < len; j++)
                            result |= buffer[i + j] << (8 * j);
                        ret.push_back(T(result));
                    }
                    return ret;
                };
            else
                readUnit = [sampleBits, unitCount, unitBytes](const std::vector<uint8_t> &buffer)
                {
                    std::vector<T> ret;
                    size_t bits = 0;
                    uint32_t result = 0;
                    for (size_t i = 0; i < unitBytes; i++)
                    {
                        result = (result << 8) | buffer[i];
                        bits += 8;
                        if (bits >= sampleBits)
                        {
                            bits -= sampleBits;
                            ret.push_back(T(result >> bits));
                            result &= (1 << bits) - 1;
                        }
                    }
                    return ret;
                };

            if (info.tileOffset.size() > 0)
            {
                size_t realWidth = info.width + info.tileWidth - 1;
                realWidth -= realWidth % info.tileWidth;
                size_t realHeight = info.height + info.tileHeight - 1;
                realHeight -= realHeight % info.tileHeight;
                ret.resize(realWidth * realHeight * info.samplePerPixel);

                size_t ch = info.samplePerPixel;
                size_t planarCount = 1;
                if (info.planarConfig == 2)
                {
                    ch = 1;
                    planarCount = info.samplePerPixel;
                }

                size_t tilePerPlanar = info.tileOffset.size() / (realWidth / info.tileWidth) / (realHeight / info.tileHeight);
                size_t tilePerLine = realWidth / info.tileWidth;
                for (size_t p = 0; p < planarCount; p++)
                {
                    size_t planarBase = p * (realWidth * realHeight);
                    for (size_t tileIndex = 0; tileIndex < tilePerPlanar; tileIndex++)
                    {
                        file.seekg(info.tileOffset[p * tilePerPlanar + tileIndex]);
                        size_t baseR = tileIndex / tilePerLine * info.tileHeight;
                        size_t baseC = tileIndex % tilePerLine * info.tileWidth * ch;
                        for (size_t r = 0; r < info.tileHeight; r++)
                            for (size_t c = 0; c < info.tileWidth * ch; c += unitCount)
                            {
                                file.read((char *)buffer.data(), unitBytes);
                                auto units = readUnit(buffer);
                                for (size_t u = 0; u < unitCount; u++)
                                    ret[planarBase + (baseR + r) * realWidth * ch + baseC + c + u] = units[u];
                            }
                    }
                }
                return {realWidth, realHeight, ret};
            }
            else
            {
                size_t realWidth = info.width;
                size_t realHeight = info.height + info.rowsPerStrip - 1;
                realHeight -= realHeight % info.rowsPerStrip;
                ret.resize(realWidth * realHeight * info.samplePerPixel);

                size_t ch = info.samplePerPixel;
                size_t planarCount = 1;
                if (info.planarConfig == 2)
                {
                    ch = 1;
                    planarCount = info.samplePerPixel;
                }

                size_t stripPerPlanar = info.stripOffset.size() / (realHeight / info.rowsPerStrip);
                for (size_t p = 0; p < planarCount; p++)
                {
                    size_t planarBase = p * (realWidth * realHeight);
                    for (size_t stripIndex = 0; stripIndex < stripPerPlanar; stripIndex++)
                    {
                        file.seekg(info.stripOffset[p * stripPerPlanar + stripIndex]);
                        size_t baseR = stripIndex * info.rowsPerStrip;
                        for (size_t r = 0; r < info.rowsPerStrip; r++)
                            for (size_t c = 0; c < realWidth * ch; c += unitCount)
                            {
                                file.read((char *)buffer.data(), unitBytes);
                                auto units = readUnit(buffer);
                                for (size_t u = 0; u < unitCount; u++)
                                    ret[planarBase + (baseR + r) * realWidth * ch + c + u] = units[u];
                            }
                    }
                }
                return {realWidth, realHeight, ret};
            }
        }
    }
}

#endif
