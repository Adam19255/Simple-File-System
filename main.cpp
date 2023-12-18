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

#define DISK_SIZE 512

// Function to convert decimal to binary char
char decToBinary(int n) {
    return static_cast<char>(n);
}

// #define SYS_CALL
// ============================================================================
class fsInode { // save the blocks number of the saved file
    int fileSize;
    int block_in_use;
    int block_size;

    // Saved in the inode
    int directBlock1;
    int directBlock2;
    int directBlock3;

    // Pointer to a block in the disk
    int singleInDirect;

    // Pointer to a block that contains pointers to blocks
    int doubleInDirect;

    int* singleInDirectArray;
    int* firstLayerDoubleArray;
    int* secondLayerDoubleArray;
    int indexDouble;

public:
    fsInode(int _block_size) {
        fileSize = 0;
        block_in_use = 0;
        block_size = _block_size;
        directBlock1 = -1;
        directBlock2 = -1;
        directBlock3 = -1;
        singleInDirect = -1;
        doubleInDirect = -1;
        singleInDirectArray = new int[_block_size];
        firstLayerDoubleArray = new int[_block_size];
        secondLayerDoubleArray = new int[_block_size];
        for (int i = 0; i < _block_size; ++i) {
            singleInDirectArray[i] = -1;
            firstLayerDoubleArray[i] = -1;
            secondLayerDoubleArray[i] = -1;
        }
        indexDouble = -1;
    }

    int getFileSize(){
        return fileSize;
    }
    void setFileSize(int newSize) {
        fileSize = newSize;
    }

    int getBlockInUse(){
        return block_in_use;
    }
    void setBlockInUse(int newBlockInUse){
        block_in_use = newBlockInUse;
    }

    int getDirectBlock1(){
        return directBlock1;
    }
    void setDirectBlock1(int newDirectBlock1){
        directBlock1 = newDirectBlock1;
    }

    int getDirectBlock2(){
        return directBlock2;
    }
    void setDirectBlock2(int newDirectBlock2){
        directBlock2 = newDirectBlock2;
    }

    int getDirectBlock3(){
        return directBlock3;
    }
    void setDirectBlock3(int newDirectBlock3){
        directBlock3 = newDirectBlock3;
    }

    int getSingleInDirect(){
        return singleInDirect;
    }
    void setSingleInDirect(int newSingleInDirect){
        singleInDirect = newSingleInDirect;
    }

    int getDoubleInDirect(){
        return doubleInDirect;
    }
    void setDoubleInDirect(int newDoubleInDirect){
        doubleInDirect = newDoubleInDirect;
    }

    void setSingleInDirectArray(int index, int data){
        singleInDirectArray[index] = data;
    }
    int getSingleInDirectArray(int index){
        return singleInDirectArray[index];
    }

    void setFirstLayerDoubleArray(int index, int data){
        firstLayerDoubleArray[index] = data;
    }
    int getFirstLayerDoubleArray(int index){
        return firstLayerDoubleArray[index];
    }

    void setSecondLayerDoubleArray(int index, int data){
        secondLayerDoubleArray[index] = data;
    }
    int getSecondLayerDoubleArray(int index){
        return secondLayerDoubleArray[index];
    }

    void setIndexDouble(int newIndex){
        indexDouble = newIndex;
    }
    int getIndexDouble(){
        return indexDouble;
    }

    // Destructor to clean up dynamically allocated arrays
    ~fsInode() {
        delete[] singleInDirectArray;
        delete[] firstLayerDoubleArray;
        delete[] secondLayerDoubleArray;
    }
};

// ============================================================================
class FileDescriptor { // save the name of the file and the pointer to the inode of the file
    pair<string, fsInode*> file;
    bool inUse; // ture - file is open... false - file is closed

public:
    FileDescriptor(string FileName, fsInode* fsi) {
        file.first = FileName;
        file.second = fsi;
        inUse = true;
    }

    string getFileName() {
        return file.first;
    }
    void setFileName(string newFileName){
        file.first = newFileName;
    }

    fsInode* getInode() {
        return file.second;
    }

    int GetFileSize() {
        return file.second->getFileSize();
    }

    bool isInUse() {
        return (inUse);
    }

    void setInUse(bool _inUse) {
        inUse = _inUse ;
    }
};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
// ============================================================================
class fsDisk { // the disk itself, saves all the info of the disk
    FILE *sim_disk_fd;

    bool is_formated;
    int block_size;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not. (i.e. if BitVector[0] == 1 , means that the
    //              first block is occupied).
    int BitVectorSize;
    int *BitVector;

    // Unix directories are lists of association structures,
    // each of which contains one filename and one inode number.
    map<string, fsInode*>  MainDir ;

    // OpenFileDescriptors --  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    vector< FileDescriptor > OpenFileDescriptors;

    // Vector to hold the deleted inodes, so they won't show in the OpenFileDescriptors and we can deallocate them later
    vector<fsInode*> DeletedInodes;

public:
    // ------------------------------------------------------------------------
    fsDisk() {
        sim_disk_fd = fopen( DISK_SIM_FILE , "w+" );
        assert(sim_disk_fd); // making sure that the file is there, if not the program will terminate
        for (int i=0; i < DISK_SIZE ; i++) { // a loop that covers each block in the disk
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET ); // prepares to write at the current position
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd ); // '1' specifies that we're writing one unit of data, and '1' specifies the size of each unit
            assert(ret_val == 1); // checks if the fwrite operation was successful (i.e., it wrote exactly 1 unit of data). If not, the program will terminate
        }
        fflush(sim_disk_fd); // ensures that all the data written using fwrite is actually stored in the file
        is_formated = false;
    }

    // ------------------------------------------------------------------------
    void listAll() {
        if (!is_formated) {
            cout << "ERR: Disk is not formatted." << endl;
            return;
        }
        int i = 0;
        // 'auto' - allows the compiler to automatically deduce the data type of variable based on its initializer or usage
        for ( auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it) {
            cout << "index: " << i << ": FileName: " << it->getFileName() <<  " , isInUse: "
                 << it->isInUse() << " file Size: " << it->GetFileSize() << endl;
            i++;
        }
        char bufy; // declares a character variable to store the byte read from the disk
        cout << "Disk content: '" ;
        for (i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fread(  &bufy , 1 , 1, sim_disk_fd ); // reads one byte of data from the disk and stores it in the bufy variable
            cout << bufy;
        }
        cout << "'" << endl;
    }

    // ------------------------------------------------------------------------
    void fsFormat(int blockSize = 4) {
        // Check if the blockSize is in valid size
        if (blockSize % 2 != 0 || blockSize <= 0 || blockSize > DISK_SIZE){
            cout << "ERR: blockSize is not valid." << endl;
            return;
        }

        // Check if the disk is already formatted
        if (is_formated) { // If we want to format the disk again we need to delete all we had before
            // Close any open files
            for (size_t i = 0; i < OpenFileDescriptors.size(); ++i) {
                if (OpenFileDescriptors[i].isInUse()) {
                    CloseFile(i);
                }
            }
            // Clear the MainDir and delete all files
            ClearMainDir();
            // Clean up BitVector
            delete[] BitVector;
            // Clean up OpenFileDescriptors
            OpenFileDescriptors.clear();
            for (int i=0; i < DISK_SIZE ; i++) { // Format all the data in the disk
                int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
                ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd );
            }
        }

        // Calculate the required size of the bit vector based on disk size and block size
        BitVectorSize = DISK_SIZE / blockSize;
        // Creat a BitVector for the disk
        BitVector = new int[BitVectorSize];
        for (int i = 0; i < BitVectorSize; ++i) {
            BitVector[i] = 0;  // Initialize all blocks as free
        }

        // Clear the MainDir and OpenFileDescriptors since the disk is being formatted
        MainDir.clear();
        OpenFileDescriptors.clear();
        // Update the variables we will use in the code
        is_formated = true;
        this->block_size = blockSize;
        cout << "Disk formatted with block size: " << blockSize << endl;
    }

    // ------------------------------------------------------------------------
    int CreateFile(string fileName) {
        if (!is_formated) {
            cout << "ERR: Disk is not formatted. Cannot create a file." << endl;
            return -1;
        }

        // Check if the file already exists in the directory
        auto it = MainDir.find(fileName); // search for the file name in the MainDir map
        if (it != MainDir.end()) { // MainDir.find returns the end of the map if the file name was not found
            cout << "ERR: File already exists." << endl;
            return -1;
        }

        // Check if the string is valid
        if (!IsStringValid(fileName)){
            cout << "ERR: File must have a name." << endl;
            return -1;
        }

        // Removing extra spaces that might have entered
        fileName = RemoveExtraSpaces(fileName);

        // Allocate an inode for the new file
        fsInode* newInode = new fsInode(block_size);
        // Add the new file entry to MainDir
        MainDir[fileName] = newInode;

        // Search for an available file descriptor slot
        int fd = -1;
        for (size_t i = 0; i < OpenFileDescriptors.size(); ++i) {
            // If one of the files is in the OFD but not in use we will take that position
            if (!OpenFileDescriptors[i].isInUse()) {
                fd = static_cast<int>(i);
                OpenFileDescriptors[i] = FileDescriptor(fileName, newInode);
                OpenFileDescriptors[i].setInUse(true);
                break;
            }
        }

        if (fd == -1) {
            // No available file descriptor slot found, add another to the end
            OpenFileDescriptors.push_back(FileDescriptor(fileName, newInode));
            fd = static_cast<int>(OpenFileDescriptors.size()) - 1;
            OpenFileDescriptors[fd].setInUse(true);
        }

        return fd; // Return the file descriptor
    }

    // ------------------------------------------------------------------------
    int OpenFile(string FileName ) {
        if (!is_formated) {
            cout << "ERR: Disk is not formatted. Cannot open a file." << endl;
            return -1;
        }

        auto it = MainDir.find(FileName);
        if (it == MainDir.end()) {
            cout << "ERR: File not found." << endl;
            return -1;
        }

        // Check if the file is already open
        for (size_t i = 0; i < OpenFileDescriptors.size(); ++i) {
            if (OpenFileDescriptors[i].getFileName() == FileName && OpenFileDescriptors[i].isInUse()) {
                cout << "ERR: File is already open." << endl;
                return -1;
            }
        }

        // Search for an available file descriptor slot
        int fd = -1;
        for (size_t i = 0; i < OpenFileDescriptors.size(); ++i) {
            // If one of the files is in the OFD but not in use we will take that position
            if (!OpenFileDescriptors[i].isInUse()) {
                fd = static_cast<int>(i);
                OpenFileDescriptors[i] = FileDescriptor(FileName, it->second);
                OpenFileDescriptors[i].setInUse(true);
                break;
            }
        }

        if (fd == -1) {
            // No available file descriptor slot found, add another to the end
            OpenFileDescriptors.push_back(FileDescriptor(FileName, it->second));
            fd = static_cast<int>(OpenFileDescriptors.size()) - 1;
            OpenFileDescriptors[fd].setInUse(true);
        }
        return fd; // Return the file descriptor
    }

    // ------------------------------------------------------------------------
    string CloseFile(int fd) {
        if (!is_formated) {
            cout << "ERR: Disk is not formatted. Cannot close a file." << endl;
            return "-1";
        }

        if (fd < 0 || fd >= static_cast<int>(OpenFileDescriptors.size())) {
            cout << "ERR: Invalid file descriptor." << endl;
            return "-1";
        }

        FileDescriptor& fileDescriptor = OpenFileDescriptors[fd];
        string fileName = fileDescriptor.getFileName();

        if (!fileDescriptor.isInUse()) {
            cout << "ERR: File is already closed." << endl;
            return fileName;
        }

        fileDescriptor.setInUse(false);
        return fileName;
    }
    // ------------------------------------------------------------------------
    int WriteToFile(int fd, char *buf, int len ) {
        if (!is_formated) {
            cout << "ERR: Disk is not formatted. Cannot write to a file." << endl;
            return -1;
        }

        if (fd < 0 || fd >= static_cast<int>(OpenFileDescriptors.size())) {
            cout << "ERR: Invalid file descriptor." << endl;
            return -1;
        }

        FileDescriptor& fileDescriptor = OpenFileDescriptors[fd];

        if (!fileDescriptor.isInUse()) {
            cout << "ERR: File is not open. Cannot write." << endl;
            return -1;
        }

        fsInode* inode = fileDescriptor.getInode();
        int fileSize = inode->getFileSize();

        // Check if there is enough space in the file
        int availableSpace = ((3 + block_size + (block_size * block_size)) * block_size) - fileSize;
        if (availableSpace == 0){
            cout << "ERR: The file is full." << endl;
            return -1;
        }

        // Check how many free blocks are there, so we know if we can allocate to the inDirect blocks
        int freeBlocks = 0;
        for (int i = 0; i < BitVectorSize; ++i) {
            if (BitVector[i] == 0)
                freeBlocks++;
        }

        int enter; // Variable to check if we need to enter the first layer of the doubleInDirect
        int blockInUse = inode->getBlockInUse();
        int diskFull;
        bool writeAgain = false; // Bool to tell if we wrote once to this block
        bool lastBit = false; // Bool to check if we are at the last block of the disk
        // Strings to save the pointers of the indirect blocks
        string singleLayer, doubleFirstLayer, doubleSecondLayer;
        int inOffset = 0; // Offset for the indirect pointers
        // Variables for the write functions
        int ret_val, offset, bytesWritten = 0, bytesToWrite = 0;

        for (int i = 0; i < BitVectorSize; ++i) {
            diskFull = i;
            if (i + 1 == BitVectorSize && inode->getFileSize() % block_size != 0){
                lastBit = true;
            }
            if (BitVector[i] == 0 || lastBit){
                // Check if there is more space in the file
                availableSpace = ((3 + block_size + (block_size * block_size)) * block_size) - inode->getFileSize();
                if (availableSpace == 0 && len != bytesWritten) {
                    break;
                }
                else if (len != bytesWritten) {
                    // Calculate the offset and the bytes that needs to be written
                    offset = inode->getFileSize() % block_size;
                    bytesToWrite = min(len - bytesWritten, block_size - offset);
                    // If we have an empty space in one of the allocated blocks we occupy it first
                    if (offset != 0 || lastBit){
                        blockInUse--; // We lower 'blockInUse' so we will enter the correct section of the file
                        writeAgain = true;
                        if (BitVectorSize > 1){
                            i--; // We lower 'i' so it won't use a new BitVector block
                        }
                    }
                    // Allocate the main blocks
                    if (blockInUse == 0){
                        if (inode->getDirectBlock1() == -1)
                            inode->setDirectBlock1(i);
                        ret_val = fseek(sim_disk_fd, (inode->getDirectBlock1() * block_size) + offset, SEEK_SET);
                        ret_val = fwrite(buf + bytesWritten, 1, bytesToWrite, sim_disk_fd);
                    }
                    else if (blockInUse == 1){
                        if (inode->getDirectBlock2() == -1)
                            inode->setDirectBlock2(i);
                        ret_val = fseek(sim_disk_fd, (inode->getDirectBlock2() * block_size) + offset, SEEK_SET);
                        ret_val = fwrite(buf + bytesWritten, 1, bytesToWrite, sim_disk_fd);
                    }
                    else if (blockInUse == 2){
                        if (inode->getDirectBlock3() == -1)
                            inode->setDirectBlock3(i);
                        ret_val = fseek(sim_disk_fd, (inode->getDirectBlock3() * block_size) + offset, SEEK_SET);
                        ret_val = fwrite(buf + bytesWritten, 1, bytesToWrite, sim_disk_fd);
                    }
                    else if (blockInUse == 3 && inode->getSingleInDirect() == -1){
                        if (freeBlocks < 2){
                            cout << "ERR: Not enough space in disk for singleInDirect allocation." << endl;
                            break;
                        }
                        inode->setSingleInDirect(i);
                        bytesToWrite = 0;
                    }
                    else if (blockInUse > 3 + block_size && inode->getDoubleInDirect() == -1){
                        if (freeBlocks < 3){
                            cout << "ERR: Not enough space in disk for doubleInDirect allocation." << endl;
                            break;
                        }
                        inode->setDoubleInDirect(i);
                        bytesToWrite = 0;
                    }
                    // SingleInDirect allocation
                    else if (blockInUse > 3 && blockInUse <= 3 + block_size && inode->getSingleInDirect() != -1){
                        // Get the inDirect offset and add to the direct block offset if needed
                        inOffset = (blockInUse - 4) % block_size;
                        if (offset == 0) { // Only enter here if we need to write another pointer
                            inode->setSingleInDirectArray(inOffset, i);
                            singleLayer = decToBinary(i);
                            // Writing the pointers for the singleInDirect
                            ret_val = fseek(sim_disk_fd, (inode->getSingleInDirect() * block_size) + inOffset, SEEK_SET);
                            ret_val = fwrite(singleLayer.c_str(), 1, 1, sim_disk_fd);
                        }
                        // Writing the data for the singleInDirect
                        ret_val = fseek(sim_disk_fd, (inode->getSingleInDirectArray(inOffset) * block_size) + offset, SEEK_SET);
                        ret_val = fwrite(buf + bytesWritten, 1, bytesToWrite, sim_disk_fd);
                    }
                    // DoubleInDirect allocation
                    else if (blockInUse > 4 + block_size && inode->getDoubleInDirect() != -1){
                        enter = (blockInUse - 4) % (1 + block_size);
                        if (enter == 0){ // Write the doubleInDirect pointers
                            // Set the index for the offset of the first layer
                            inode->setIndexDouble(inode->getIndexDouble() + 1);
                            int index = inode->getIndexDouble(); // Get that index so we can use it
                            doubleFirstLayer = decToBinary(i);
                            // Record the position of the pointers
                            inode->setFirstLayerDoubleArray(index, i);
                            ret_val = fseek(sim_disk_fd, (inode->getDoubleInDirect() * block_size) + index, SEEK_SET);
                            ret_val = fwrite(doubleFirstLayer.c_str(), 1, 1, sim_disk_fd);
                            bytesToWrite = 0;
                        }
                        else{ // Allocate the singleInDirect of the doubleInDirect pointers
                            inOffset = enter - 1; // Offset for the second layer
                            if (offset == 0) {
                                doubleSecondLayer = decToBinary(i);
                                // Record the position of the pointers
                                inode->setSecondLayerDoubleArray(inOffset, i);
                                ret_val = fseek(sim_disk_fd, (inode->getFirstLayerDoubleArray(inode->getIndexDouble()) * block_size) + inOffset, SEEK_SET);
                                ret_val = fwrite(doubleSecondLayer.c_str(), 1, 1, sim_disk_fd);
                            }
                            ret_val = fseek(sim_disk_fd, (inode->getSecondLayerDoubleArray(inOffset) * block_size) + offset, SEEK_SET);
                            ret_val = fwrite(buf + bytesWritten, 1, bytesToWrite, sim_disk_fd);
                        }
                    }
                    // Set blockInUse, update BitVector
                    blockInUse++;
                    inode->setBlockInUse(blockInUse);
                    if (writeAgain)
                        writeAgain = false;
                    else
                        freeBlocks--;
                    BitVector[i] = 1;
                    bytesWritten += bytesToWrite;
                    inode->setFileSize(bytesWritten + fileSize); // Update the file size in the inode
                }
                else
                    break;
            }
        }
        // If we got here then there are no more free blocks in the BitVector and the disk if full
        if (diskFull + 1 == BitVectorSize && bytesWritten != len){
            cout << "ERR: Disk is full." << endl;
            return -1;
        }

        return 1;
    }

    // ------------------------------------------------------------------------
    int DelFile(string FileName) {
        if (!is_formated) {
            cout << "ERR: Disk is not formatted. Cannot delete a file." << endl;
            return -1;
        }
        // Check if the file exists
        auto it = MainDir.find(FileName);
        if (it == MainDir.end()) {
            cout << "ERR: File not found." << endl;
            return -1;
        }

        // Check if the file is open and remove it from OpenFileDescriptors
        for (auto openFileIt = OpenFileDescriptors.begin(); openFileIt != OpenFileDescriptors.end(); ++openFileIt) {
            if (openFileIt->getFileName() == FileName && openFileIt->isInUse()) {
                cout << "ERR: File is open. Close it before deleting." << endl;
                return -1;
            }
            if (openFileIt->getFileName() == FileName) { // Deleting the name of the file
                openFileIt->setFileName("");
            }
        }

        fsInode* inode = it->second;

        // Free the directBlocks occupied by the file in the BitVector
        if (inode->getDirectBlock1() != -1) {
            BitVector[inode->getDirectBlock1()] = 0;
        }
        if (inode->getDirectBlock2() != -1) {
            BitVector[inode->getDirectBlock2()] = 0;
        }
        if (inode->getDirectBlock3() != -1) {
            BitVector[inode->getDirectBlock3()] = 0;
        }

        // Free blocks in singleInDirect
        if (inode->getSingleInDirect() != -1) {
            BitVector[inode->getSingleInDirect()] = 0; // Free the pointer area
            for (int i = 0; i < block_size; ++i) { // Free the data area
                int blockNum = inode->getSingleInDirectArray(i);
                if (blockNum != -1) {
                    BitVector[blockNum] = 0;
                }
            }
        }

        // Free blocks in doubleInDirect
        if (inode->getDoubleInDirect() != -1) {
            BitVector[inode->getDoubleInDirect()] = 0; // Free the double pointers
            for (int i = 0; i < block_size; ++i) {
                int singleBlockNum = inode->getFirstLayerDoubleArray(i);
                if (singleBlockNum != -1) {
                    BitVector[singleBlockNum] = 0; // Free the single pointers
                    char* indirectBlock = new char[block_size]; // Buffer to hold the data read from the inDirect block
                    // Initialize the indirectBlock with null characters
                    memset(indirectBlock, '\0', block_size);
                    // Read the pointers of the singleInDirect into the array
                    fseek(sim_disk_fd, singleBlockNum * block_size, SEEK_SET);
                    fread(indirectBlock, 1, block_size, sim_disk_fd);
                    // Free the data
                    for (int j = 0; j < block_size; ++j) {
                        int blockNum = (unsigned char)indirectBlock[j]; // Convert back to int
                        if (blockNum != '\0') {
                            BitVector[blockNum] = 0;
                        }
                    }
                    delete[] indirectBlock; // Delete the dynamically allocated array
                }
            }
        }
        inode->setFileSize(0); // Zero out the file size, so we won't get a problem in valgrind
        // Remove the file entry from the MainDir and put it in the delete vector to delete later
        MainDir.erase(it);
        DeletedInodes.push_back(inode);

        return 1; // Return success
    }
    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len) {
        if (!is_formated) {
            cout << "ERR: Disk is not formatted. Cannot read from a file." << endl;
            buf[0] = '\0'; // Null-terminate so we won't get garbage values when reading failed
            return -1;
        }

        if (fd < 0 || fd >= static_cast<int>(OpenFileDescriptors.size())) {
            cout << "ERR: Invalid file descriptor." << endl;
            buf[0] = '\0';
            return -1;
        }

        FileDescriptor& fileDescriptor = OpenFileDescriptors[fd];

        if (!fileDescriptor.isInUse()) {
            cout << "ERR: File is not open. Cannot read." << endl;
            buf[0] = '\0';
            return -1;
        }

        fsInode* inode = fileDescriptor.getInode();

        // Ensure we are not trying to read more than we have
        int bytesToRead = min(len, inode->getFileSize());
        int bytesRead = 0;

        // 'blockNum' (the block number to read from), 'count' (the number of bytes to read from the block)
        auto readFromBlock = [&](int blockNum, int count) {
            fseek(sim_disk_fd, blockNum * block_size, SEEK_SET);
            fread(buf + bytesRead, 1, count, sim_disk_fd);
            bytesRead += count;
        };

        // Read from direct blocks
        if (bytesRead < bytesToRead) {
            if (inode->getDirectBlock1() != -1) {
                readFromBlock(inode->getDirectBlock1(), min(block_size, bytesToRead - bytesRead));
            }
            if (inode->getDirectBlock2() != -1 && bytesRead < bytesToRead) {
                readFromBlock(inode->getDirectBlock2(), min(block_size, bytesToRead - bytesRead));
            }
            if (inode->getDirectBlock3() != -1 && bytesRead < bytesToRead) {
                readFromBlock(inode->getDirectBlock3(), min(block_size, bytesToRead - bytesRead));
            }
        }

        // Read from singleInDirect block
        if (bytesRead < bytesToRead && inode->getSingleInDirect() != -1) {
            for (int i = 0; i < block_size && bytesRead < bytesToRead; i++) {
                if (inode->getSingleInDirectArray(i) != -1)
                    readFromBlock(inode->getSingleInDirectArray(i), min(block_size, bytesToRead - bytesRead));
            }
        }
        // Read from doubleInDirect block
        if (bytesRead < bytesToRead && inode->getDoubleInDirect() != -1) {
            for (int i = 0; i < block_size && bytesRead < bytesToRead; i++) {
                if (inode->getFirstLayerDoubleArray(i) != -1) {
                    // Buffer to hold the data read from the inDirect block
                    char *indirectBlock = new char[block_size];
                    fseek(sim_disk_fd, inode->getFirstLayerDoubleArray(i) * block_size, SEEK_SET);
                    fread(indirectBlock, 1, block_size, sim_disk_fd);

                    for (int j = 0; j < this->block_size && bytesRead < bytesToRead; j++) {
                        readFromBlock((unsigned char) indirectBlock[j], min(block_size, bytesToRead - bytesRead));
                    }
                    delete[] indirectBlock;
                }
            }
        }

        buf[bytesRead] = '\0';
        return 1;
    }

    // ------------------------------------------------------------------------
    int CopyFile(string srcFileName, string destFileName) {
        if (!is_formated) {
            cout << "ERR: Disk is not formatted. Cannot copy a file." << endl;
            return -1;
        }

        // Check if source and destination file names are the same
        if (srcFileName == destFileName) {
            cout << "ERR: Source and destination filenames are the same." << endl;
            return -1;
        }

        // Check if the source file exists
        auto srcFileIt = MainDir.find(srcFileName);
        if (srcFileIt == MainDir.end()) {
            cout << "ERR: Source file not found." << endl;
            return -1;
        }

        int srcFileDescriptor = -1;
        // Check if the source file is open
        for (size_t i = 0; i < OpenFileDescriptors.size(); ++i) {
            if (OpenFileDescriptors[i].getInode() == srcFileIt->second && OpenFileDescriptors[i].isInUse()) {
                cout << "ERR: Source file must be closed to copy." << endl;
                return -1;
            }
        }
        int destBlocks = 0;
        // Check if the destination file exists
        auto it = MainDir.find(destFileName);
        if (it != MainDir.end()) {
            destBlocks = it->second->getBlockInUse();
            // Check if the destination file is open
            for (size_t i = 0; i < OpenFileDescriptors.size(); ++i) {
                if (OpenFileDescriptors[i].getInode() == it->second && OpenFileDescriptors[i].isInUse()) {
                    cout << "ERR: Destination file must be closed." << endl;
                    return -1;
                }
            }
        }

        // Check if there is enough space on the disk to complete the copy
        int freeBlocks = 0;
        for (int i = 0; i < BitVectorSize; ++i) {
            if (BitVector[i] == 0){
                freeBlocks++;
            }
        }
        if (srcFileIt->second->getBlockInUse() > freeBlocks + destBlocks){
            cout << "ERR: Not enough space to complete copy." << endl;
            return -1;
        }

        // After we past all the test we start the actual copy
        // Open the source file for copying
        srcFileDescriptor = OpenFile(srcFileName);
        fsInode* srcInode = srcFileIt->second;
        // Get the file size of the source file
        int srcFileSize = srcInode->getFileSize();
        // Allocate a buffer to hold the source file's contents
        char* buffer = new char[srcFileSize + 1]; // +1 for null-termination
        // Read the contents of the source file into the buffer
        ReadFromFile(srcFileDescriptor, buffer, srcFileSize);
        CloseFile(srcFileDescriptor); // Close the file after we read what we want to copy

        if (it != MainDir.end()) // If the destination file exists we delete it for overwrite
            DelFile(destFileName);

        // Create a new file with the destination name
        int destFileDescriptor = CreateFile(destFileName);

        // Write the contents of the buffer to the destination file
        WriteToFile(destFileDescriptor, buffer, srcFileSize);
        delete[] buffer; // Free the buffer
        // Close the file
        CloseFile(destFileDescriptor);

        return 1;
    }

    // ------------------------------------------------------------------------
    int RenameFile(string oldFileName, string newFileName) {
        if (!is_formated) {
            cout << "ERR: Disk is not formatted. Cannot rename a file." << endl;
            return -1;
        }

        // Check if the old file exists
        auto oldIt = MainDir.find(oldFileName);
        if (oldIt == MainDir.end()) {
            cout << "ERR: Old file not found." << endl;
            return -1;
        }

        // Check if the old file is open
        size_t index = -1;
        for (size_t i = 0; i < OpenFileDescriptors.size(); ++i) {
            if (OpenFileDescriptors[i].getFileName() == oldFileName) {
                if (OpenFileDescriptors[i].isInUse()) {
                    cout << "ERR: Old file is open. Close it before renaming." << endl;
                    return -1;
                }
                else {
                    index = i;
                    break;
                }
            }
        }

        // Check if the new file name is valid
        if (!IsStringValid(newFileName)){
            cout << "ERR: The new file name is not valid." << endl;
            return -1;
        }

        // Removing extra spaces that might have entered
        newFileName = RemoveExtraSpaces(newFileName);

        // Check if the new file name is already in use
        auto newIt = MainDir.find(newFileName);
        if (newIt != MainDir.end()) {
            // Checking to see if we are trying to rename the same file to the same name
            if (oldIt->second == newIt->second){
                return 1;
            }
            cout << "ERR: New file name already in use." << endl;
            return -1;
        }

        fsInode* oldInode = oldIt->second;

        // Create a new entry with the new file name and the same inode and change the name is the OFD
        if (index != -1)
            OpenFileDescriptors[index].setFileName(newFileName);
        MainDir[newFileName] = oldInode;
        // Remove the old entry
        MainDir.erase(oldIt);

        return 1;
    }
    // ------------------------------------------------------------------------
    int GetFileSize(int fd) {
        if (!is_formated) {
            cout << "ERR: Disk is not formatted. Cannot write to a file." << endl;
            return -1;
        }

        if (fd < 0 || fd >= static_cast<int>(OpenFileDescriptors.size())) {
            cout << "ERR: Invalid file descriptor." << endl;
            return -1;
        }

        return OpenFileDescriptors[fd].getInode()->getFileSize();
    }

    // ------------------------------------------------------------------------
    ~fsDisk() {
        // Close any open files
        for (size_t i = 0; i < OpenFileDescriptors.size(); ++i) {
            if (OpenFileDescriptors[i].isInUse()) {
                CloseFile(i);
            }
        }

        // Clear the MainDir and delete all files
        ClearMainDir();

        // Clean up BitVector
        delete[] BitVector;

        // Clean up OpenFileDescriptors
        OpenFileDescriptors.clear();

        // Close the simulated disk file
        fclose(sim_disk_fd);
    }

    // Function to help clear the MainDir
    void ClearMainDir() {
        // List of filenames to delete
        vector<string> filesToDelete;
        for (auto it = MainDir.begin(); it != MainDir.end(); ++it) {
            filesToDelete.push_back(it->first);
        }

        // Delete the files from the list
        for (const string& fileName : filesToDelete) {
            DelFile(fileName);
        }

        // Delete the inodes
        while (!DeletedInodes.empty()){
            delete DeletedInodes.back(); // Activate the destructor
            DeletedInodes.pop_back(); // Remove from the vector
        }
    }

    // ------------------------------------------------------------------------
    // Function to check if the string is valid
    bool IsStringValid(string& str) {
        // Check if the string is empty
        if (str.empty()) {
            return false;
        }

        // Check if the string contains only whitespace characters
        for (char c : str) {
            if (!isspace(static_cast<unsigned char>(c))) {
                return true; // Found a non-whitespace character
            }
        }

        return false; // String contains only whitespace characters
    }
    // ------------------------------------------------------------------------
    // Function to remove extra spaces from a string
    string RemoveExtraSpaces(string& input) {
        string result = input;

        // Find the first non-space character from the beginning
        size_t startPos = result.find_first_not_of(' ');

        if (startPos != string::npos) {
            // Find the last non-space character from the end
            size_t endPos = result.find_last_not_of(' ');

            // Erase the extra spaces
            result = result.substr(startPos, endPos - startPos + 1);
        }

        return result;
    }
};

// ============================================================================
int main() {
    int blockSize;
    string fileName;
    string fileName2;
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
            case 0:   // delete all the disk and exit
                delete fs;
                exit(0);
                break;

            case 1:  // print all files in disk - list-file
                fs->listAll();
                break;

            case 2:    // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;

            case 3:    // creat-file and opens it
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 4:  // open-file, needs to return -1 as an int if there is any problem
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5:  // close-file, needs to return -1 as a string if there is any problem
                cin >> _fd;
                fileName = fs->CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 6:   // write-file, check if there is enough space in the disk, file, that the file is open and that the disk is formatted, if not return -1
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

            case 8:   // delete file, need also to delete the inode of this file
                cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 9:   // copy file, open file can be copied, copied file is closed by default
                cin >> fileName;
                cin >> fileName2;
                fs->CopyFile(fileName, fileName2);
                break;

            case 10:  // rename file, can't change an open file name
                cin >> fileName;
                cin >> fileName2;
                fs->RenameFile(fileName, fileName2);
                break;

            default:
                break;
        }
    }
}