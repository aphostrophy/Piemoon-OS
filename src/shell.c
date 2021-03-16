#include "shell.h"
#include "utilities.h"

void cwd(int pathIdx, char *dir) {
    int depth = 0;
    char pathOrder[64];
    char *absolutePath;
    int currDir = pathIdx;
    char file[512 * 2];
    int i, z, idx = 0;
    int dirindex=0;
    interrupt(0x21, 0, "~", 0, 0);
    *(dir+dirindex) = '~'; dirindex++;
    if((unsigned char)currDir != 0xFF) {
        interrupt(0x21, 2, file, 0x101, 0);
        interrupt(0x21, 2, file + 512, 0x102, 0);

        // backtracking to root
        while(currDir != 0xFF) {
            pathOrder[depth] = currDir;
            depth++;
            currDir = file[currDir << 4];
        }
        *(absolutePath+idx) = '/';
        idx++;

        while(depth != 0) {
            depth--;
            i = 0;
            while(file[(pathOrder[depth] << 4) + 2 + i] != 0) {
                *(absolutePath+idx) = file[(pathOrder[depth] << 4) + 2 + i];
                idx++;
                i++;
            }
            if(depth != 0) {
                *(absolutePath+idx) = '/';
                idx++;
            }
        }

        *(absolutePath+idx) = 0x00;
        interrupt(0x21, 0, absolutePath, 0, 0);
        for(z=0;z<idx;z++){
            *(dir+dirindex+z) = absolutePath[z];
        }
        dirindex += idx;
    }
    interrupt(0x21, 0, "$ ", 0, 0);
    *(dir+dirindex) = '$';
    *(dir+dirindex+1) = ' ';
    *(dir+dirindex+2) = 0x0;
}

int getPathIdx(int parentIdx, char *filename) { //Get Index file di files
    char file[512 * 2];
    char currFile[14];
    int i;
    interrupt(0x21, 2, file, 0x101, 0);
    interrupt(0x21, 2, file + 512, 0x102, 0);

    if(*(filename) == '~') {
        return 0xFF;
    } else if(*(filename) == '.' && *(filename+1) == '.') {
        if((unsigned char)parentIdx == 0xFF) {
            return -1;
        } else {
            return file[parentIdx << 4];
        }
    } else if(*(filename) == '.') {
        return parentIdx;
    } else {
        i = 0;
        while(i < 1024) {
            strslice(file, currFile, i+2, i+16);
            if(strcmp(filename, currFile, strlen(filename)) && (unsigned char)parentIdx == file[i]) {
                if((unsigned char)file[i+1] == 0xFF) {
                    return div(i,16);
                } else {
                    return -1;
                }
            }
            i += 16;
        }
        return -1;
    }
}

int cd(int currParentIdx, char *dirPath) {
    char *pathList[64];
    int parentIdx;
    int i, j, idx, depth;
    i = 0;
    idx = 0;
    depth = 0;
    
    while(*(dirPath+i) != 0x0) {
        if(*(dirPath+i) == '/') {
            while(idx < 14) {
                *(pathList[depth] + idx) = 0x0;
                idx++;
            }
            idx = 0;
            depth++;
        } else {
            *(pathList[depth] + idx) = *(dirPath+i);
            idx++;
        }
        i++;
    }
    
    while(idx < 14) {
        *(pathList[depth] + idx) = 0x0;
        idx++;
    }

    parentIdx = currParentIdx;
    for(j = 0; j <= depth; j++) {
        parentIdx = getPathIdx(parentIdx, pathList[j]);
        if(parentIdx == -1) {
            break;
        }
    }
    return parentIdx;
}

//usage : ln -s original_file linked_file
// status code : 0 (success), -1 (original_file error) , -2 (linked_file error)
int ln(char *filepath, char *filelink,int soft,unsigned char parentIndex){
    char files[1024];
    char sectors[512];
    char pathsfile[16][64];
    char pathslink[16][64];
    int countfile = strsplit(filepath,'/',pathsfile);
    int countlink = strsplit(filelink,'/',pathslink);
    int idx;
    char output[100];

    if(soft){
        interrupt(0x21,0, "ln soft dipanggil hahaha\r\n",0,0);
        idx = getPathIdx(parentIndex,filepath);
        itoa(idx,10,output);
        interrupt(0x21,0,output,0,0);
        interrupt(0x21,0,"\r\n",0,0);
        interrupt(0x21,0,filepath,0,0);
        interrupt(0x21,0,"\r\n",0,0);
        if(getPathIdx(parentIndex, filepath)==-1){
            interrupt(0x21,0, "original file not found\r\n",0,0);
            return -1;
        }
        if(getPathIdx(parentIndex, filelink)==-1){
            interrupt(0x21,0, "destination folder not found\r\n",0,0);
            return -2;
        }
    } else{
        interrupt(0x21,0, "ln hard dipanggil hahaha\r\n",0,0);
        // idx = getPathIdx(parentIndex,filepath);
        // interrupt(0x21,0, )
        if(getPathIdx(parentIndex, filepath)==-1){
            interrupt(0x21,0, "original file not found\r\n",0,0);
            return -1;
        }
        if(getPathIdx(parentIndex, filelink)==-1){
            interrupt(0x21,0, "destination folder not found\r\n",0,0);
            return -2;
        }
    }
    return 0;
}

void ls(unsigned char parentIndex)
{
  char files[1024];
  char listFiles[64];
  char name[14];
  int i = 0,j = 0,total;
  char * filenames[64];
  interrupt(0x21, 2, files, 0x101, 0);
  interrupt(0x21, 2, files+512, 0x102, 0);
  while(i<64)
  {
    if(files[i*0x10] == parentIndex && files[i*0x10 + 2] != '\0')
    {
      *(listFiles+j) = i;
      j++;  
    }
    ++i;
  }
  total = j;
  for(i = 0; i<total;i++)
  {
    clear(name,14);
    for(j=0;j<14;j++)
    {
      name[j] = files[listFiles[i]  * 0x10 + 2 + j];
    }
    // filenames[i] = name;
    interrupt(0x21, 0, name, 0, 0);
    interrupt(0x21, 0, "\n", 0, 0);
    interrupt(0x21, 0, "\r", 0, 0);
  }
}

void cat(char * filenames, char parentIdx)
{
    char *buff[512 * 16];
    int idx = strlen(filenames);
    int pathIdx;
    // while(idx < 14) {
    //     *(filenames + idx) = 0x0;
    //     idx++;
    // }
    // pathIdx = getPathIdx(parentIdx, filenames);
    // if((unsigned char)pathIdx == parentIdx)
    // {
    //     interrupt(0x21, 4, buff, pathIdx, 0);
    //     interrupt(0x21, 0, " adalah file\r\n", 0, 0);
    //     interrupt(0x21, 0, buff , 0, 0);
    //     interrupt(0x21, 0, "\r\n" , 0, 0);
    //     return;
    // }
    // interrupt(0x21, 0, filenames , 0, 0);
    // interrupt(0x21, 0, " bukan file\r\n", 0, 0);

}

void autoComplete(char *filename, char parentIdx) {
    char files[512 * 2];
    char *temp;
    char *currFilename;
    int i, idxFilename, idxTemp;
    interrupt(0x21, 2, files, 0x101, 0);
    interrupt(0x21, 2, files + 512, 0x102, 0);

    i = 0;
    while(i < 1024) {
        strslice(files, currFilename, i+2, i+16);
        if(files[i] == parentIdx && strcmp(filename, currFilename, strlen(filename))) {
            strslice(files, temp, i+2+strlen(filename), i+16);
            interrupt(0x21, 0, temp, 0, 0);

            //update filename
            idxFilename = strlen(filename);
            idxTemp = 0;
            while(*(temp+idxTemp) != 0x0) {
                *(filename+idxFilename) = *(temp+idxTemp);
                idxFilename++;
                idxTemp++;
            }
            *(filename+idxFilename) = 0x0;
            break;
        }
        i += 16;
    }
}
void mkdir( char *filenames,unsigned char parentIndex)
{
    char files[1024];
    int i = 0, InitIdx;
    int j = 0;
    interrupt(0x21, 2, files, 0x101, 0);
    interrupt(0x21, 2, files+512, 0x102, 0);
    while(i<0x40)
    {
        if(files[i * 0x10] == parentIndex && (strcmp(filenames,files + (i * 16) + 2 ,14) == 1) )
        {
            interrupt(0x21, 0, "Folder sudah ada \r\n", 0, 0);
            return;
        }
        i++;
    }

    
    while(files[j*0x10+2] != '\0' )
    {
        j++;
    } 
    if(j <= 0 || j >= 64)
    {
        interrupt(0x21, 0, "Sudah tidak bisa ditambah \r\n", 0, 0);
    }
    else
    {
        InitIdx = j * 0x10;
        files[InitIdx] = parentIndex;
        files[InitIdx + 1] = 0xFF;
        for(i = 0 ;i < 14; i++)
        {
            if(filenames[i] == '\0')
                break;
            files[InitIdx + i + 2] = filenames[i];
        }
    }
    interrupt(0x21, 3, files, 0x101, 0);
    interrupt(0x21, 3, files+512, 0x102, 0);

    // interrupt(0x21, 0, filenames, 0, 0);
}


void shell(){
    int i, j, commandCount, count, idx;
    int tabPressed = 0, arrowPressed = 0;
    char input[128];
    char temp[128];
    char command[8][64];
    int parentIdx = 0xFF;
    int targetDir;
    char dir[128];
    int status;
        
    while(1){
        if(!tabPressed && !arrowPressed) {
            cwd(parentIdx,dir);
        }

        interrupt(0x21,1,input,0,0);
        commandCount = strsplit(input,' ',command);

        if(*(command[commandCount-1]+strlen(command[commandCount-1])-1) == 0x09) {

            //catet posisi terakhir input
            count = 0;
            for(i = 0; i < commandCount; i++) {
                count += strlen(command[i]);
            }
            count += commandCount - 2;

            //catet posisi terakhir command
            j = strlen(command[commandCount-1])-1;

            *(command[commandCount-1]+strlen(command[commandCount-1])-1) = 0x0;
            autoComplete(command[commandCount-1], parentIdx);
            
            //update input
            i = count;
            while(j < strlen(command[commandCount-1])) {
                *(input+i) = *(command[commandCount-1]+j);
                i++;
                j++;
            }

            *(input+i) = 0x0;
            for(idx = 0; idx < strlen(input); idx++) {
                *(temp+idx) = *(input+idx);
            }
            *(temp+strlen(input)) = 0x0;

            //overwrite input, add payload to input[0] and input[1]
            *(input) = 0xFF;
            *(input+1) = 0xFF;
            strslice(temp, input+2, 0, strlen(temp));
            *(input+2+strlen(temp)) = 0x0;

            tabPressed = 1;
        } else if((input[0] == 0x00 && input[1] != 0x00)) {
            interrupt(0x21, 0, "arrow keteken", 0, 0);
            arrowPressed = 1;
        } else {
            if(strcmp(command[0], "cd", strlen(command[0])) && strlen(command[0])==2){
                interrupt(0x21,0, "Cd dipanggil hahaha\r\n",0,0);
                targetDir = cd(parentIdx,command[1]);
                if(targetDir != -1) {
                    parentIdx = targetDir;
                } else {
                    interrupt(0x21,0, "No such directory\r\n",0,0);
                }
            } else if(strcmp(command[0], "ls", strlen(command[0])) && strlen(command[0])==2 ){
                interrupt(0x21,0, "Ls dipanggil hahaha\r\n",0,0);
                ls((unsigned char)parentIdx);
            } else if(strcmp(command[0],"cat",strlen(command[0])) && strlen(command[0])==3 ){
                interrupt(0x21,0, "Cat dipanggil hahaha\r\n",0,0);
                // interrupt(0x21,0, command[1],0,0);
                cat(command[1],parentIdx);
            } else if(strcmp(command[0],"mkdir",strlen(command[0])) && strlen(command[0])==5){
                mkdir(command[1],parentIdx);
            } else if(strcmp(command[0],"ln",strlen(command[0])) && strlen(command[0])==2){
                if(strcmp(command[1],"-s",strlen(command[1])) && strlen(command[1])==2){
                    status = ln(command[2],command[3],1,parentIdx);
                } else{
                    status = ln(command[1],command[2],0,parentIdx);
                }
            }else{
                interrupt(0x21,0, command[0],0,0);
                interrupt(0x21,0," not found\r\n",0,0);
            }
            tabPressed = 0;
            arrowPressed = 0;
        }
        // clear(input,128);
    }
}