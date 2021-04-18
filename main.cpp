#include "intelhex.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <string>
#include <sstream>
#include <cstring>
#include <algorithm>

using namespace std;

int calculateCheckSum(string data, int bytes){
    int sum = 0;
    // Calculate the checkSum so it can be compared to the read in checkSum
    for (int i = 0; i < (bytes + 4); i++){
        sum += stoi(data.substr(1 + (i * 2), 2), nullptr, 16);
    }
    // Calculate complement of two
    sum = ((sum ^ 0xFF) + 1) & 0xFF;
    return sum;
}

void printStorage(uint8_t *destination, int readBytes){
    if(readBytes == -1){
        cout << endl << "We were not able to extract any data." << endl;
        cout << "Please check if the given file is using intel hex encoding.";
        return;
    }
    // prints all the undamaged pieces of data
    for(int i = 0; i<586; i++){
        if(destination[i] != -1){
            cout<<destination[i];
        }
    }
    cout << endl << "We were able to extract " << readBytes << " bytes of data.";
}

int readHexFile(const char *path, uint8_t *destination, int max_length){
    // this functions trys to read as much undamaged data as possible
    // even if there are damaged pieces of data it was keep reading until finished
    std::fstream fs;
    string data;
    int readBytes = -1;
    fs.open(path, std::fstream::in);
    if (fs.is_open()){
        readBytes = 0;
        while (getline(fs, data)){
            // check if line begins with :, and line length is at least 11 long
            if ((data.substr(0, 1) != ":") || (data.length() < 11)){
                continue;
            }
            // stoi(data, nullptr, 16) converts hex_string to integer
            try {
                // if trying to read the data throws an exception line is skipped
                int address = stoi(data.substr(3, 4), nullptr, 16);
                int recordType = stoi(data.substr(7, 2), nullptr, 16);
                int bytesInLine = stoi(data.substr(1, 2), nullptr, 16);
                int checkSum = stoi(data.substr(9 + (2 * bytesInLine), 2), nullptr, 16);

                int calcSum = calculateCheckSum(data, bytesInLine);

                // if checkSums are diffrent -> data was incorrect
                if (checkSum == calcSum){
                    for (int i = 0; i < bytesInLine; i++){
                        // check if adress is less than max_length so we don't read in to much data
                        if (address < (max_length - 1)){
                            // read data and increase readBytes counter
                            destination[address + i] = stoi(data.substr(9 + (i * 2), 2), nullptr, 16);
                            readBytes++;
                        }
                        else{
                            break;
                        }
                    }
                }
                else{
                    continue;
                }
                if (recordType == 0x01)
                    break;
            }catch (...) {
                continue;
            }
        }
        // recordType = 1 means that it's the end of the file
        fs.close();
    }
    if(readBytes == 0){
        return -1;
    }
    return readBytes;
}

int writeHexFile(const char *path, uint8_t *source, int startaddress, int length){
	std::ofstream ofs;
	ofs.open("test.txt", std::ofstream::out | std::ofstream::trunc);
	ofs.close();
    std::ofstream fs;
    int writtenBytes = -1;
    int bytesOfData = length;
    int recordType = 0;
    int byteCount = 16;
    int address = startaddress;
    int checkSum = 0;
    uint8_t *hexData = (uint8_t *)calloc(16, sizeof(uint8_t));
	string hexString = "";
    fs.open(path, ios::out | ios::binary | ios::app);
    if(fs.is_open()){
        writtenBytes = 0;
        while(bytesOfData >= 16){
			hexString = "";
            for(int i = address; i < (address + 16); i++){
                std::stringstream ss;
				ss << std::hex << (int)source[i];
				std::string mystr = ss.str();
				std::transform(mystr.begin(), mystr.end(),mystr.begin(), ::toupper);
				if(mystr.length()==1){
					mystr = "0" + mystr;
				}
				hexString.append(mystr);
            }
			std::stringstream ss;
			ss << std::hex << byteCount << setw(4) << setfill('0') << address << setw(2) << setfill('0') << recordType;
			std::string mystr = ss.str();
			mystr.append(hexString);
			mystr = ":" + mystr;
			checkSum = calculateCheckSum(mystr, 16);
			fs << ":" << setw(2) << hex <<setfill('0') << byteCount << setw(4) << setfill('0') << address << setw(2) << setfill('0') << recordType << std::uppercase<< hexString << setw(2) << setfill('0') << checkSum << "\n";
			//fs << ":" << setw(2) << setfill('0') << uppercase << byteCount << setw(4) << setfill('0') << address << setw(2) << setfill('0') << recordType << hexData << setw(2) << setfill('0') << checkSum << "\n";
            bytesOfData -= 16;
			address += 16;
            writtenBytes += 16;
        }
        byteCount = bytesOfData;
        if(byteCount > 0){
			hexString = "";
            for(int i = address; i < (address + byteCount); i++){
				std::stringstream ss;
				ss << std::hex << (int)source[i];
				std::string mystr = ss.str();
				std::transform(mystr.begin(), mystr.end(),mystr.begin(), ::toupper);
				if(mystr.length()==1){
					mystr = "0" + mystr;
				}
				hexString.append(mystr);
                //hexData[i - address] = source[i];
            }
			std::stringstream ss;
			ss << std::hex << setw(2) << setfill('0') << byteCount << setw(4) << setfill('0') << address << setw(2) << setfill('0') << recordType;
			std::string mystr = ss.str();
			mystr.append(hexString);
			mystr = ":" + mystr;
			checkSum = calculateCheckSum(mystr, byteCount);
			fs << ":" << setw(2) << uppercase<< hex<<setfill('0') << byteCount << setw(4) << setfill('0') << address << setw(2) << setfill('0') << recordType << uppercase<< hexString << setw(2) << setfill('0') << checkSum << "\n";
			//fs << ":" << setw(2) << setfill('0') << uppercase << byteCount << setw(4) << setfill('0') << address << setw(2) << setfill('0') << recordType << hexData << setw(2) << setfill('0') << checkSum << "\n";
            address += byteCount;
            bytesOfData -= byteCount;
            writtenBytes += byteCount;
        }
        fs << ":00000001FF";
        fs.close();
    }
    free(hexData);
    return writtenBytes;
}

int main(){
    // mallocs space in the storage which can be used safely
    uint8_t *storage = (uint8_t *)calloc(586, sizeof(uint8_t));
    int readBytes = readHexFile("quickbrownfox.hex", storage, 586);
    // print the number of read bytes
    printStorage(storage, readBytes);
    int writtenBytes = writeHexFile("test.txt", storage, 0, 586);
    // prints number of written bytes
    if (writtenBytes > -1){
        cout << endl << "Writing was successful." << endl;
        cout << writtenBytes << " bytes of data were written to the file." << endl;
    }else{
        cout << endl << "Writing was unsuccessful." << endl;
        cout << "There was a problem while trying to write to the file." << endl;
    }
    // frees the space which was malloced before
    free(storage);
    return 0;
}