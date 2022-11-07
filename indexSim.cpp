#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 256

// ============================================================================
void decToBinary(int n, char &c)
{
    // array to store binary number
    int binaryNum[8];

    // counter for binary array
    int i = 0;
    while (n > 0)
    {
        // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }

    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--)
    {
        if (binaryNum[j] == 1)
            c = c | 1u << j;
    }
}



// ============================================================================

class FsFile {
    int file_size;
    int block_in_use;
    int index_block;
    int block_size;

public:
    FsFile(int _block_size) {
        file_size = 0;
        block_in_use = 0;
        block_size = _block_size;
        index_block = -1;
    }

    int getfile_size(){
        return file_size;
    }
    void setfile_size(int size){
        file_size=size;
    }

    int getindex_block(){
        return index_block;
    }
    void setindex_block (int indexBlock){
        index_block=indexBlock;
    }

    int getblock_in_use(){
        return block_in_use;
    }
    void setblock_in_use(int block){
        block_in_use=block;
    }

};

// ============================================================================

class FileDescriptor {
    string file_name;
    FsFile* fs_file;
    bool inUse;

public:

    FileDescriptor(string FileName, FsFile* fsi) {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
    }

    string getFileName() {
        return file_name;
    }

    bool getIsUse(){
        return inUse;
    }

    void setInUse(bool inUseValue){
        inUse=inUseValue;
    }

    FsFile *getFsFile(){
        return fs_file;
    }

};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"

// ============================================================================

class fsDisk {
    FILE *sim_disk_fd;
    bool is_formated;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.
    int BitVectorSize;
    int *BitVector;

    // (5) MainDir --
    // Structure that links the file name to its FsFile
    vector <FileDescriptor *> MainDir;

    // (6) OpenFileDescriptors --
    //  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    vector <FileDescriptor *> OpenFileDescriptors;

    int block_size;
    int free_blocks;
    int max_size;

public:
    // ------------------------------------------------------------------------
    //constrctor
    fsDisk() {
        sim_disk_fd = fopen(DISK_SIM_FILE , "r+");
        assert(sim_disk_fd);

        for (int i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd);
            assert(ret_val == 1);
        }

        fflush(sim_disk_fd);
        is_formated = false;
        block_size=0;
        free_blocks=0;
        max_size=0;
    }

    // check if to close all files.
    //destructor
    ~fsDisk(){
        int i;
        for (i=0; i<MainDir.size();i++){
            delete MainDir[i]->getFsFile();
            delete MainDir[i];
        }
        delete BitVector;
        fclose(sim_disk_fd);
    }

    // ------------------------------------------------------------------------
    /**
     * lists all the disk content
     */
    void listAll() {
        int i = 0;

        //ALL FILE DESCRIPTORS
        for (i=0;i<MainDir.size();i++) {
            cout << "index: " << i << ": FileName: " << MainDir[i]->getFileName()  <<  " , isInUse: " <<MainDir[i]->getIsUse()<< endl;
        }

        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++)
        {
            cout << "(";
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread(&bufy, 1, 1, sim_disk_fd);
            cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }

    // ------------------------------------------------------------------------
    /**
    * formats the disk depending on the blocksize
    * @param blockSize
    */
    void fsFormat( int blockSize =4 ) {
        // initialize Sizes and calculations
        block_size=blockSize;
        BitVectorSize=DISK_SIZE/this->block_size; // the blocks amount calculation based on the user input.
        free_blocks=BitVectorSize;
        BitVector= new int [this->BitVectorSize];
        max_size=this->block_size*this->block_size; // max size calculation.

        //delete all items file descriptor in the main directory.
        for(FileDescriptor *fd:MainDir){
            delete fd->getFsFile();
            delete fd;
        }
        MainDir.clear();// remove all values form the MainDir
        OpenFileDescriptors.clear(); // remove all values for the OFD

        // Zero the Disk
        int zeroVal;
        int i;
        for(i=0;i<DISK_SIZE;i++){
            fseek(sim_disk_fd,i,SEEK_SET);
            zeroVal=(int) fwrite("\0",1,1,sim_disk_fd);
            assert(zeroVal==1);
        }
        for(i=0;i<BitVectorSize;i++){
            BitVector[i]=0;
        }

        // change format flag to true;
        is_formated=true;
    }

    // ------------------------------------------------------------------------
    /**
     * create a new file and opens it.
     * @param fileName
     * @return fd of new file on success, -1 on failure
     */
    int CreateFile(string fileName) {
        // if file is not formatted, exit function with error.
        if(is_formated == false){
            return -1;
        }

        // create Metadata for the new file
        FsFile *newFS=new FsFile(block_size);
        // create FD for new file
        FileDescriptor *newFD=new FileDescriptor(fileName,newFS);

        // find an empty block and add the file to it.
        int bitVectorEmpty;
        for (bitVectorEmpty=0;bitVectorEmpty<BitVectorSize;bitVectorEmpty++){
            if(BitVector[bitVectorEmpty]==0){
                BitVector[bitVectorEmpty]=1;
                free_blocks=free_blocks-1;
                break;
            }
        }
        newFS->setindex_block(bitVectorEmpty);//set index block to the found empty block in the bit vector.

        // add file to the main Dir
        MainDir.push_back(newFD);
        // add to OpenFileDescriptors
        OpenFileDescriptors.push_back(newFD);
        newFD->setInUse(true);

        return OpenFileDescriptors.size()-1;
    }

    // ------------------------------------------------------------------------
    /**
    * opens a closed file.
    * @param fileName
    * @return open file descripton on success, -1 on failure
    */
    int OpenFile(string fileName) {
        // if file is not formatted, exit function with error.
        if(is_formated == false){
            return -1;
        }

        // check if file is available in mainDir
        bool found=false;
        int i;
        for(i=0;i<MainDir.size();i++){
            if(MainDir[i]->getFileName()==fileName){
                found=true;
                // if found check if it is open
                if(MainDir[i]->getIsUse()== true){
                    return -1;
                }
                else{
                    MainDir[i]->setInUse(true); // make it open
                    OpenFileDescriptors.push_back(MainDir[i]); // add it to openFDs
                    return OpenFileDescriptors.size()-1;
                }
            }
        }
        // file not found!
        if (found== false){
            return -1;
        }
        return -1;

    }

    // ------------------------------------------------------------------------
    /**
     * closes an open file.
     * @param fd
     * @return filename on success, -1 on failure
     */
    string CloseFile(int fd) {
        // if file is not formatted, exit function with error.
        if(is_formated == false){
            return "-1";
        }
        // check if fd is legal
        if(fd<0 || fd<OpenFileDescriptors.size()-1){
            return "-1";
        }

        string fileName=OpenFileDescriptors[fd]->getFileName();
        OpenFileDescriptors[fd]->setInUse(false);
        OpenFileDescriptors.erase(OpenFileDescriptors.begin()+fd);
        return fileName;
    }
    // ------------------------------------------------------------------------
    /**
     * writes data on the disk
     * @param fd
     * @param buf
     * @param len
     * @return 0 on success, -1 on failure
     */
    int WriteToFile(int fd, char *buf, int len ) {

        // if file is not formatted, exit function with error.
        if(is_formated == false){
            return -1;
        }
        // check if fd is legal
        if(fd<0){
            return -1;
        }


        int indexBlock=OpenFileDescriptors[fd]->getFsFile()->getindex_block();

        int fileSize=OpenFileDescriptors[fd]->getFsFile()->getfile_size();
        if(fileSize+len>max_size){
            return -1;
        }
        int blockUsed=OpenFileDescriptors[fd]->getFsFile()->getblock_in_use();

        int i;
        free_blocks=free_blocks-len;


        int availableSpace=(blockUsed*block_size)-fileSize;


        if (availableSpace>=len){
            int existingBlock= findExistingBlock(indexBlock,blockUsed);
            int offset=block_size-availableSpace;
            char c;

            fseek(sim_disk_fd,(existingBlock+1)*block_size,SEEK_SET);
            fread(&c,1,1,sim_disk_fd);

            int i;
            for(i=0;i<len;i++){
                writer(&buf[i],1,existingBlock,offset+i);
            }
            writer(&c,1,existingBlock,offset+i);
            int usedSpace=block_size-availableSpace;
            int bitVectorOffset=(block_size*existingBlock)+usedSpace;
            for(i=0;i<len;i++){
                BitVector[bitVectorOffset+i]=1;
            }




        } else {
            int allocBlocks= ceilVal((len-availableSpace),block_size);
            int bufferOffset=0;
            free_blocks=free_blocks-allocBlocks;

            // check if there is an empty space in existing block
            if (availableSpace != 0) {
                int existingBlock = findExistingBlock(indexBlock, blockUsed);
                int offset = block_size - availableSpace;
                char c;
                fseek(sim_disk_fd,(existingBlock+1)*block_size,SEEK_SET);
                fread(&c,1,1,sim_disk_fd);
                int i;
                for (i = 0; i < availableSpace; i++) {
                    writer(&buf[i], 1, existingBlock, offset + i);
                }
                writer(&c,1,existingBlock,offset+i);
                bufferOffset = i;
                int usedSpace = block_size - availableSpace;
                int bitVectorOffset = (block_size * existingBlock) + usedSpace;
                for (i = 0; i < len; i++) {
                    BitVector[bitVectorOffset + i] = 1;
                }
            }

            int i;
            int k;
            int j = 0;
            for (i = 0; i < allocBlocks; i++) {

                // create a new block and write to index block
                int block = findEmptyBlock();
                char blockStr = toChar(block);

                writer(&blockStr, 1, indexBlock, blockUsed + i); // write the block location to the index block
                BitVector[block] = 1;


                while (j < block_size && j < len) {
                    writer(&buf[bufferOffset + j], 1, block, j);
                    j++;
                }

            }
            OpenFileDescriptors[fd]->getFsFile()->setblock_in_use(blockUsed + allocBlocks);

        }

        OpenFileDescriptors[fd]->getFsFile()->setfile_size(fileSize + len);
        return 0;
    }
        // ------------------------------------------------------------------------
    /**
     * deletes a file
     * @param FileName
     * @return 0 on success, -1 on failure
     */
    int DelFile( string FileName ) {
        // if file is not formatted, exit function with error.
        if(is_formated == false){
            return -1;
        }
        bool found=false;
        int i;
        int j;
        int k;
        for(i=0;i<MainDir.size();i++){
            if(MainDir[i]->getFileName()==FileName) {
                found = true;


                int blocksUsed = MainDir[i]->getFsFile()->getblock_in_use();
                int indexBlock = MainDir[i]->getFsFile()->getindex_block();
                for (j = 0; j < blocksUsed; j++) {
                    int block = findExistingBlock(indexBlock, j + 1);
                    for (k = 0; k < block_size; k++) {
                        int offset = block_size * block;
                        BitVector[offset + k] = 0;
                    }
                }
                for (j = 0; j < block_size; j++) {
                    int offset = block_size * indexBlock;
                    BitVector[offset + j] = 0;
                }

                for (j = 0; j < OpenFileDescriptors.size(); j++) {
                    if (OpenFileDescriptors[j]->getFileName() == FileName) {
                        CloseFile(j);
                    }
                    free_blocks = free_blocks + blocksUsed + 1;
                    delete MainDir[i]->getFsFile();
                    delete MainDir[i];
                    MainDir.erase(MainDir.begin() + i);
                    return 1;
                }
            }
        }
        // file not found!
        if (found== false){
            return -1;
        }
        return -1;


    }
    // ------------------------------------------------------------------------
    /**
     * reads data from file
     * @param fd
     * @param buf
     * @param len
     * @return 0 on success, -1 on failure
     */
    int ReadFromFile(int fd, char *buf, int len ) {
        // if file is not formatted, exit function with error.
        if(is_formated == false){
            return -1;
        }
        // check if fd is legal
        if(fd<0){
            return -1;
        }
        int indexBlock=OpenFileDescriptors[fd]->getFsFile()->getindex_block();
        int blocksUsed=OpenFileDescriptors[fd]->getFsFile()->getblock_in_use();
        int blocksToRead=ceilVal(len,block_size);
        if(blocksToRead>blocksUsed){
            blocksToRead=blocksUsed;
        }
        int i;
        char str[DISK_SIZE];
        int index=0;

        for (i=0;i<blocksToRead;i++){
            int block= findExistingBlock(indexBlock,i+1);
            int offset=block_size*block;
            int j;
            for(j=0;j<block_size;j++){
                fseek(sim_disk_fd,offset+j,SEEK_SET);
                fread(&str[index],1,1,sim_disk_fd);
                index++;
            }
        }

        str[len]='\0';
        strcpy(buf,str);
        return 0;

    }

    /**
     * takes input and block details and writes to that area
     * @param buf
     * @param len
     * @param block
     * @param moveOffset
     */
    void writer( char *buf,int len,int block,int moveOffset){
        int offset = (block*block_size)+moveOffset;
        fseek(sim_disk_fd,offset,SEEK_SET);
        int i;
        for (i=0; i < len+1 ; i++) {
            int ret_val = fseek ( sim_disk_fd , offset+i , SEEK_SET );
            ret_val = fwrite( &buf[i] ,  1 , 1, sim_disk_fd);
            assert(ret_val == 1);
        }
    }
    /**
     * finds the first empty block in BitVector
     * @return
     */
    int findEmptyBlock(){
        int emptyBlock;
        int i;
        for(i=0;i<BitVectorSize;i++){
            if(BitVector[i]==0){
                emptyBlock=i;
                break;
            }
        }
        return emptyBlock;
    }

    /**
     * finds what block data is written to according to the index block and the block number.
     * @param indexBlock
     * @param blockNumber
     * @return
     */
    int findExistingBlock(int indexBlock,int blockNumber){
        char bufy;
        int offset=(indexBlock*block_size)+blockNumber;

        fseek(sim_disk_fd,offset-1,SEEK_SET);
        fread(&bufy,1,1,sim_disk_fd);
        int block= toInt(&bufy);
        return block;
    }

    /**
     * converts int to char
     * @param a
     * @return
     */
    char toChar(int a){
        char str=a+'0';
        return str;
    }

    /**
     * converts char to int
     * @param str
     * @return
     */
    int toInt(char *str){
        char convert=str[0];
        return convert-'0';
    }

    /**
     * divides and rounds up
     * @param a
     * @param b
     * @return
     */
    int ceilVal(int a,int b){
        return (a/b)+((a%b)!=0);
    }

};

int main() {
    int blockSize;
    int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;


    fsDisk *fs = new fsDisk();
    int cmd_;
    while(1) {
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
                delete fs;
                exit(0);
                break;

            case 1:  // list-file
                fs->listAll();
                break;

            case 2:    // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;

            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile( _fd , str_to_write , strlen(str_to_write) );
                break;

            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read ;
                fs->ReadFromFile( _fd , str_to_read , size_to_read );
                cout << "ReadFromFile: " << str_to_read << endl;
                break;

            case 8:   // delete file
                cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }

}