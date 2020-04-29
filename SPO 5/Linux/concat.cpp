#include <iostream>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <aio.h>
#include <string.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 80

using namespace std;

bool isTxt(char* str){

    int i = 0;
    char search[] = "txt.";

    while(str[i+1]) i++;

    for(int j = 0; j < 4; j++)
        if(str[i-j] != search[j]) return 0;

    return 1;
}

bool isValid(char* str){

    if(!isTxt(str))
        return 0;

    if(!strcmp(str, "output.txt"))
        return 0;

    return  1;
}

void* writer (void* ptr){

    int pipe = *(int*)ptr;                                                    

    char buf[BUF_SIZE];                                                        
    ssize_t symRead;                                                           
  
    int sem = semget(ftok("./main.cpp", 1), 0, IPC_CREAT | 0666);
    struct sembuf wait = {0, 0, SEM_UNDO};                                     
    struct sembuf lock = {0, 1, SEM_UNDO};                                     
    struct sembuf unlock = {1, -1, SEM_UNDO};                                  
    semop(sem, &lock, 1);

    int file;                                                                  
    file = open("output.txt", O_WRONLY | O_TRUNC | O_CREAT);                   

    aiocb writeFile;                                                          
    writeFile.aio_fildes = file;                                              
    writeFile.aio_offset = 0;                                                  
    writeFile.aio_buf = &buf;                                                  


    while(1){

        semop(sem, &wait, 1);                                                  
        semop(sem, &lock, 1);

        symRead = read(pipe, buf, BUF_SIZE);                                   

        if(!symRead) break;

        writeFile.aio_nbytes = symRead;                                        
        aio_write(&writeFile);                                                 

        while(aio_error(&writeFile) == EINPROGRESS);                           

        writeFile.aio_offset += symRead;                                       

        semop(sem, &unlock, 1);                                                
    }

}

void* reader (void* ptr){

    int pipe = *(int*)ptr;                                                     

    char buf[BUF_SIZE];                                                        

    int sem = semget(ftok("./main.cpp", 1), 2, IPC_CREAT | 0666);              
    struct sembuf wait = {1, 0, SEM_UNDO};                                     
    struct sembuf lock = {1, 1, SEM_UNDO};                                     
    struct sembuf unlock = {0, -1, SEM_UNDO};                                  

    aiocb readFile;                                                            
    readFile.aio_buf = &buf;                                                   

    DIR* directory;                                                            
    dirent* nextFile;                                                          
    directory = opendir("/home/valera/CLionProjects/SPO 5");                               

    int file;                                                                  
    struct stat stat;                                                          
    int size;                                                                  

    while(1){

        nextFile = readdir(directory);                                         

        if(nextFile == NULL) break;                                            

        if(!isValid(nextFile->d_name))                                         
            continue;

        file = open(nextFile->d_name, O_RDONLY);                               

        fstat(file, &stat);                                                   
        size = stat.st_size;                                                   

        readFile.aio_fildes = file;                                            
        readFile.aio_offset = 0;                                               


        while(1){

            if(size > BUF_SIZE) readFile.aio_nbytes = BUF_SIZE;
            else readFile.aio_nbytes = size;

            aio_read(&readFile);                                               

            while(aio_error(&readFile) == EINPROGRESS);                        

            semop(sem, &wait, 1);                                              
            semop(sem, &lock, 1);
           
            write(pipe, buf, readFile.aio_nbytes);                             

            semop(sem, &unlock, 1);                                            

            if(size > BUF_SIZE){
                size -= BUF_SIZE;
                readFile.aio_offset += BUF_SIZE;
            }
            else   break;   
        }

        close(file);                                                           
    }

    semop(sem, &wait, 1);                                                      
}
 
extern "C" void concat(){

    int hndl[2];                                                              

    if(pipe(hndl)){
        cout << "Failed to create pipe!" << endl;
        return;
    }

    pthread_t writer_thread;         
    if(pthread_create(&writer_thread, NULL, writer, &hndl[0])){

        cout << "Failed to create writer_thread" << endl;
        return;
    }   

    pthread_t reader_thread;         
    if(pthread_create(&reader_thread, NULL, reader, &hndl[1])){

       cout << "Failed to create reader_Thread" << endl;
       pthread_cancel(writer_thread);
       return;
    }

    pthread_join(reader_thread, NULL); 
    
    pthread_cancel(writer_thread);                                             

    return;
}
