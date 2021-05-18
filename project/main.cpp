#include <iostream>

#include "imageupdater.h"

using namespace std;

int main(int argc, char* argv[]){
    if(argc >= 2){
        ImageUpdater img1(argv[1]);
        cout << "sha256 " << (img1.checkImageFile() ? " OK " : "FAIL") << endl;
        if(argc == 3){
            cout << "partition " << (img1.checkPartition(argv[2]) ? " OK " : "FAIL") << endl;
        }
    }else{
        ImageUpdater img1("/home/s13/RPE/OpenWRT_Black/update.json");
        cout << "sha256 " << (img1.checkImageFile() ? " OK " : "FAIL") << endl;
        img1.getRootPart();
    }
    return 0;
}
