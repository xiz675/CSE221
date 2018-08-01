#pragma once
#include "rely.h"

#if defined(_WIN32)
#   include <winioctl.h>

struct RawFile
{
    HANDLE File;
    RawFile(const std::string& fileName, const bool isNoBuffer = false, const bool isSharing = false)
    {
        File = CreateFileA(fileName.c_str(), GENERIC_READ, isSharing ? (FILE_SHARE_READ | FILE_SHARE_WRITE) : NULL, NULL, OPEN_EXISTING, isNoBuffer ? FILE_FLAG_NO_BUFFERING : FILE_ATTRIBUTE_NORMAL, nullptr);
        if (File == INVALID_HANDLE_VALUE)
            throw std::runtime_error(("can not CreeatFile --- " + std::to_string(GetLastError())).c_str());
    }
    ~RawFile()
    {
        CloseHandle(File);
    }
    void Seek(const uint64_t offset) const
    {
        LARGE_INTEGER tmp;
        tmp.QuadPart = (LONGLONG)offset;
        SetFilePointer(File, tmp.LowPart, &tmp.HighPart, FILE_BEGIN);
    }
    void Read(void* ptr, const uint32_t size) const
    {
        DWORD dummy;
        ReadFile(File, ptr, size, &dummy, nullptr);
    }
};

struct FileMapping : public RawFile
{
    uint64_t FileSize;
    HANDLE Mapping;
    const uint32_t* Data;
    FileMapping(const std::string& fileName) : RawFile(fileName)
    {
        DWORD sizeLo, sizeHi;
        sizeLo = GetFileSize(File, &sizeHi);
        FileSize = sizeLo + ((uint64_t)(sizeHi) << 32);
        Mapping = CreateFileMapping(File, nullptr, PAGE_READONLY, 0, 0, nullptr);
        Data = (const uint32_t*)MapViewOfFile(Mapping, FILE_MAP_READ, 0, 0, 0);
    }
    ~FileMapping()
    {
        UnmapViewOfFile(Data);
        CloseHandle(Mapping);
    }
};

struct RawDisk : public RawFile
{
    uint64_t Size;
    uint32_t SectorSize;
    RawDisk(const std::string& diskName) : RawFile(diskName, false, true)
    {
        DISK_GEOMETRY_EX diskGeo;
        DWORD dummy;
        DeviceIoControl(File, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskGeo, sizeof(diskGeo), &dummy, nullptr);
        Size = diskGeo.DiskSize.QuadPart;
        SectorSize = diskGeo.Geometry.BytesPerSector;
    }
};

#else
#   include <fcntl.h>
#   include <sys/mman.h>
#   include <sys/ioctl.h>
#   if defined(__APPLE__)
#       include <sys/disk.h>
#   else
#       include <linux/fs.h>
#   endif
struct RawFile
{
    int32_t File;
    RawFile(const std::string& fileName, const bool isNoBuffer = false, const bool isSharing = false)
    {
        int flag = O_RDONLY;
        #if !defined(__APPLE__)
        if (isNoBuffer)
            flag |= O_DIRECT;
        #endif
        File = open(fileName.c_str(), flag);
        #if defined(__APPLE__)
        if (isNoBuffer)
            fcntl(File, F_NOCACHE, 1);
        #endif
        if (File == -1)
            throw std::runtime_error(("can not Open File --- " + std::to_string(errno)).c_str());
    }
    ~RawFile()
    {
        close(File);
    }
    void Seek(const uint64_t offset) const
    {
        lseek(File, offset, SEEK_SET);
    }
    void Read(void* ptr, const uint32_t size) const
    {
        read(File, ptr, size);
    }
};

struct FileMapping : public RawFile
{
    uint64_t FileSize;
    const uint32_t* Data;
    FileMapping(const std::string& fileName) : RawFile(fileName)
    {
        FileSize = lseek(File, 0, SEEK_END);
        lseek(File, 0, SEEK_SET);
        Data = (const uint32_t*)mmap(nullptr, FileSize, PROT_READ, MAP_PRIVATE, File, 0);
    }
    ~FileMapping()
    {
        munmap((void*)Data, FileSize);
    }
};

struct RawDisk : public RawFile
{
    uint64_t Size;
    uint32_t SectorSize;
    RawDisk(const std::string& diskName) : RawFile(diskName, false, true)
    {
    #if defined(__APPLE__)
        ioctl(File, DKIOCGETBLOCKSIZE, &SectorSize);
        unsigned long nblocks = 0;
        ioctl(File, DKIOCGETBLOCKCOUNT, &nblocks);
        Size = (uint64_t)nblocks * SectorSize;
    #else
        ioctl(File, BLKGETSIZE64, &Size);
        ioctl(File, BLKSSZGET, &SectorSize);
    #endif
    }
};

#endif


