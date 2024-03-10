# Operating System Multi-Process Executing

This is a short application made with C programming language, that its goal is to sort the records inside a specific file, and to print them to the tty terminal. The sorting process though is a little different from the usual ones. The real goal of the application is to test multi-process executing, the OS provides and to combine this idea with the first goal, to sort different files of records. The application works as follows:

## How to download the application
To download the application you can either download the [zip file](https://github.com/AntonisZks/Multi-Process-Sorting/archive/refs/heads/main.zip) containing all the code and data files, or clone its url using the following command:
```
git clone https://github.com/AntonisZks/Multi-Process-Sorting.git
```

## How to run the application
When you have downloaded the application, open up a tty terminal, locate the directory containing the application and execute the following commands:
```
make all
make run_test
```
On that moment if a list of records has appeared on the tty, then everything worked fine, and you are ready to play with the application.
> [!IMPORTANT]
> Because of the fact that this application uses specific Linux libraries and functions, the compilation of the program will work only on an Ubuntu Linux Operating System.

## How the application works
The general execute command of the application is the following:
```
./bin/mysort -i <data-filename> -k <number-of-sub-processes> -e1 <sorting-algorithm-1> -e2 <sorting-algorithm-2>
```
where each of the above is descibed below:
- The main process of the application is called `Coordinator-Spliter and Merger-Reporter` and its main job is to take a specific file of records that need to be sorted, and split the sorting process into a specific number of sub-processes.
- Each of the sub-processes' name is `Work-Spliter and Result-Merger` and they have to sort a specific range of records inside the file. This range of records for each Work-Spliter has been approved by the parent process, the Coordinator-Spliter.
> [!NOTE]
> For example if the number of records is n, and the sub-processes are k, then each Sub-Process has to sort n/k records.
- Although the spiting is not done yet. Every Work-Spliter splits its sorting process into individual sub-processes. So now the root process has sub-sub-processes called `Sorters`. Every sorter has a specific range of records inside the file to sort and that has been approved by its parent process the Work-Spliter in the same way that was described above.
> [!NOTE]
> Every Work-Spliter has a specific number of sub-processes. If the number of Work-Spliters is k then the first one will have k child-processes, the second one will have k-2, the third one k-3, ... , and the k-th one will have 1 child-process. 
