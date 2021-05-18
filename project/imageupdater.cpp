#include "imageupdater.h"

bool quickFSEntryExists(string name){
    struct stat stat_data;
    int fd = stat(name.c_str(), &stat_data);
    if(-1 == fd)
        return false;
    return true;
}

ImageUpdater::ImageUpdater(const string jsonFilename){
    Json::CharReaderBuilder crBuilder;
    Json::Value jsonRoot;
    JSONCPP_STRING jsonErrors;
    ifstream jsonFile;

    /// TODO consider addign private constructor for inits

    size_t found = jsonFilename.find_last_of("/\\");
    folder = jsonFilename.substr(0, found+1);

    vector<string> stringKeys{litSha256, litImgFilename, litPartType, litVersion};

    isValidJson = false;
    isValidImage = false;
    isSuccessfulUpdate = false;

    jsonFile.open(jsonFilename);

    if(jsonFile){
        cout << "Image file exist" << endl;
    }else{
        cout << "Image file does not exist" << endl;
        return;
    }

    crBuilder["collectComments"] = false;
    if(!parseFromStream(crBuilder, jsonFile, &jsonRoot, &jsonErrors)){
        cout << "Failed to parse: " << jsonFilename << endl << jsonErrors << endl;
        return;
    }

    for(auto k: stringKeys)
        if(!jsonRoot[k].isString())
            return;

    if(!jsonRoot[litSizeBytes].isUInt())
        return;

    sizeBytes = jsonRoot[litSizeBytes].asUInt64();

    imgFilename = jsonRoot[litImgFilename].asString();
    sha256sum = jsonRoot[litSha256].asString();
    version = jsonRoot[litVersion].asString();

    string tmpPartType = jsonRoot[litPartType].asString();
    if(tmpPartType == litPartTypeRoot){
        partType = PartType::Root;
    }else if(tmpPartType == litPartTypeBoot){
        partType = PartType::Boot;
    }else{
        /// TODO Consider deeming this an invalid state
        partType = PartType::Unknown;
    }

    auto tmpTargets = jsonRoot[litTargets];
    if(!tmpTargets.isArray() || tmpTargets.size() != 2)
        return;

    for(auto i: tmpTargets){
        auto partName =  i.asString();
        if(!quickFSEntryExists(partName)){
            // partition does not exist
            return;
        }
        targets.push_back(partName);
    }

    this->isValidJson = true;
}

string ImageUpdater::sha256FromFile(const string filename){
    unsigned char buf[4096]; /// TODO get block size from filesystem
    unsigned char digest[SHA256_DIGEST_LENGTH];
    struct stat stat_data;
    SHA256_CTX sha_context;
    uint64_t sz_file, sz_read;
    ssize_t bytes;
    int fd;

    fd = stat(filename.c_str(), &stat_data);
    if(-1 == fd)
        return "";

    sz_file = stat_data.st_size;
    sz_read = 0L;
    /// TODO check for size

    fd = open(filename.c_str(), O_RDONLY);
    if(-1 == fd)
        return "";

    SHA256_Init(&sha_context);
    bytes = read(fd, buf, 4096);
    sz_read += bytes;
    while(bytes > 0){
        SHA256_Update(&sha_context, buf, bytes);
        bytes = read(fd, buf, 4096);
    }

    SHA256_Final(digest, &sha_context);
    std::ostringstream oss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
          oss << std::hex << std::setw(2) << std::setfill('0') << +digest[i];

    return oss.str();
}

bool ImageUpdater::checkSha256(const string filename, const string summ){
    return sha256FromFile(filename) == summ;
}

bool ImageUpdater::checkImageFile(){
    string path = folder + "/" + imgFilename;
    isValidImage = checkSha256(path, sha256sum);
    return isValidImage;
}

bool ImageUpdater::checkPartition(const string devName){
    return checkSha256(devName, sha256sum);
}


///
/// TODO Consider rewriting this twisted function
///
/// Rules work like this:
/// - + If entry name starts with any denied prefix -- it is skiped
///   |
///   + Else + if prefixAllowed not empty, and entry name starts with any of the allowed prefixes -- it is to be added
///          |
///          + else it is just added
/// /
list<string> ImageUpdater::listDir(string path, vector<string> prefixesAllowed, vector<string> prefixesDenied){
    list<string> entries;
    DIR *dir;
    struct dirent *ent;
    enum class EStatus{unknown, add, skip} status;

    if((dir = opendir(path.c_str())) != NULL){
        while ((ent = readdir (dir)) != NULL){
            status = EStatus::unknown;

            string entry(ent->d_name);
            if(!prefixesDenied.empty()){
                for(auto prefix : prefixesDenied)
                    if(entry.rfind(prefix, 0) == 0)
                        status = EStatus::skip;
            }
            if(status == EStatus::unknown && !prefixesAllowed.empty()){
                for(size_t i = 0; i < prefixesAllowed.size() && status == EStatus::unknown; ++i){
                    if(entry.rfind(prefixesAllowed[i], 0) == 0){
                        status = EStatus::add;
                    }
                }
            }

            if(prefixesAllowed.empty() && status != EStatus::skip){
                entries.push_back(entry);
            }else if(status == EStatus::add){
                entries.push_back(entry);
            }
        }
        closedir (dir);
    }
    return entries;
}



string ImageUpdater::getPartByMajorMinor(string majorMinor){
    auto disks = listDir(litSysBlock, {}, {"loop", "."});
    majorMinor += "\n"; // Because kernel representation has '\n' at the end
    for(auto disk : disks){
        auto partitions = listDir(litSysBlock + disk + "/", {disk});
        for(auto partition : partitions){
            auto partition_full = litSysBlock + disk + "/" + partition;
            ifstream devFile(partition_full + "/dev");
            if(devFile){
                string str((std::istreambuf_iterator<char>(devFile)), std::istreambuf_iterator<char>());
                if(str == majorMinor){
                    return partition;
                }
            }
        }
    }

    return "";
}

string ImageUpdater::getRootPart(){
    const char* litRoot = "/";
    struct stat stat_data;
    __dev_t dev_major, dev_minor;
    string dev_combined;

    int fd = stat(litRoot, &stat_data);
    if(-1 == fd){
        return "";
    }

    // TODO test this part for endiannesssafety
    dev_minor = 0xFF & stat_data.st_dev;
    dev_major = (stat_data.st_dev - dev_minor) / 0xFF;

//    cout << static_cast<unsigned>(dev_major) << ":" << static_cast<unsigned>(dev_minor) << endl;

    dev_combined = std::to_string(static_cast<unsigned>(dev_major)) + ":" +
            std::to_string(static_cast<unsigned>(dev_minor));

    return getPartByMajorMinor(dev_combined);
}

string ImageUpdater::getTargetPart(){
    string rootDevice;
    rootDevice = getRootPart();
    if(targets.size() != 2)
        return "";
    if(targets[0] == rootDevice)
        return targets[1];
    return targets[0];
}


bool ImageUpdater::readyToWrite(){
    if(!isValidJson || !isValidImage)
        return false;

    string targetTmp = getTargetPart();
    if(!quickFSEntryExists(targetTmp))
        return false;

    // TODO Add more checks here

    deductedTargetPartition = targetTmp;
    return true;
}

bool ImageUpdater::writePartition(){
    blkid_probe pr;
    blkid_loff_t blkDevSize;
    uint64_t sizePartition, sizeCopied;
    ssize_t io_r, io_w;
    uint32_t fdImage, fdPartition;

    uint8_t* buf = (uint8_t*)malloc(FSBlockSize);
    if(nullptr == buf){
        cout << "Failed to allocate memory" << endl;
        return false;
    }

    fdImage = open(this->imgFilename.c_str(), O_RDONLY);
    if(fdImage < 0){
        cout << "Failed to open an image file" << endl;
        return false;
    }

    fdPartition = open(deductedTargetPartition.c_str(), O_RDWR);
    if(fdPartition < 0){
        cout << "Failed to open an partition" << endl;
        return false;
    }

    pr = blkid_new_probe_from_filename(deductedTargetPartition.c_str());
    blkDevSize = blkid_probe_get_size(pr);

    sizePartition = static_cast<size_t>(blkDevSize);
    sizeCopied = 0;

    while(sizeCopied < sizePartition){
        io_r = read(fdImage, buf, FSBlockSize);
        if(io_r == -1){
            cout << "Read operation failed durign update, you better check the device" << endl;
            return false;
        }
        io_w = write(fdPartition, buf, FSBlockSize);
        if(io_w == -1 || io_r != io_w){
            cout << "Write operation failed durign update, you better check the device" << endl;
            return false;
        }
        sizeCopied += io_w;
    }

    return checkPartition(deductedTargetPartition);
}
