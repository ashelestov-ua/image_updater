#ifndef IMAGEUPDATER_H
#define IMAGEUPDATER_H

#include <list>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include <streambuf>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <blkid/blkid.h>
#include <openssl/sha.h>
#include <jsoncpp/json/json.h>

using namespace std;


class ImageUpdater{
public:
    ImageUpdater(const string jsonFilename);
    string sha256FromFile(const string filename);
    bool checkSha256(const string filename, const string summ);
    bool checkImageFile();
    bool checkPartition(const string devName);
    bool readyToWrite();
    bool writePartition();
    bool switchBootLoader();    /// TODO Implement me

    string getTargetPart();     /// TODO Implement me
    string getRootPart();       /// TODO Implement me
    string getPartByMajorMinor(string majorMinor);

    list<string> listDir(string path, vector<string> prefixAllowed = {}, vector<string> prefixDenied = {});

private:
    /// TODO consider adding a separate thread for updater

    enum class PartType{
        Unknown = 0,
        Boot,
        Root,
        Other
    };

    vector<string> targets;
    string imgFilename;
    string sha256sum;
    string version;
    string folder;
    string destDevice;
    uint64_t sizeBytes;
    PartType partType;

    string deductedTargetPartition;

    bool isValidJson;
    bool isValidImage;
    bool isSuccessfulUpdate;

    const char* litSha256       = "sha256";
    const char* litImgFilename  = "imgFile";
    const char* litSizeBytes    = "sizeBytes";
    const char* litPartType     = "partType";
    const char* litVersion      = "version";
    const char* litTargets      = "targets";

    const char* litPartTypeRoot = "root";
    const char* litPartTypeBoot = "boot";

    const string litSysBlock    = "/sys/block/";

    const size_t FSBlockSize      = 4096; // TODO deduct from actual FS later
};

#endif // IMAGEUPDATER_H
