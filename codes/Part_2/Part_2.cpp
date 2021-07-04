// Can BEYAZNAR
// 161044038
#include <iostream>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <sys/types.h>
#include <bits/stdc++.h>
#include <vector>
#include <pthread.h>

#define _INPUT_COUNT_ 8
#define MAX_INT_SIZE 16

using namespace std;

typedef struct _PageTableEntry
{
	//int val;

	//int CachingDisabled;
	bool isReferenced;
	bool isModified;
	//string Protection;
	int isPresent;
	int toPhysical_index;

} PageTableEntry;

typedef struct _LRU_struct
{
	int day;
	int hour;
	int minute;
	int second;

	int id;

} LRU_struct;

FILE *user_file = NULL;

pthread_mutex_t mutex_sort_thread;

// statistic values
int statistic_arr[6][6] =
	{0, 0, 0, 0, 0, 0,	// fill
	 0, 0, 0, 0, 0, 0,	// quick
	 0, 0, 0, 0, 0, 0,	// bubble
	 0, 0, 0, 0, 0, 0,	// merge
	 0, 0, 0, 0, 0, 0,	// index
	 0, 0, 0, 0, 0, 0};	// check

int frameSize = 0;
int numPhysicalFrames = 0;
int numVirtualFrames = 0;

char *pageReplacement = NULL;
int pageReplacement_type = 0;
char *allocPolicy = NULL;
char *fileName = NULL;
int pageTablePrintInt = 0;

int *PhysicalMem = NULL;
int PhysicalMem_size = 0;

int memoryaccess_counter = 0;

PageTableEntry **virtualMem = NULL;
int VirtualMem_size = 0;

// tname types
const char fill_str[10] = "fill";
const char quick_str[10] = "quick";
const char bubble_str[10] = "bubble";
const char merge_str[10] = "merge";
const char index_str[10] = "index";
const char check_str[10] = "check";

// Page replacement structures...
queue<int> fifo_queue;		// this will be used for FIFO and Second Chance FIFO
vector<int> nru_wsc_vector; // this will be used for NRU and WSClock
vector<LRU_struct> LRU_vector;
int WSClock_currentpoint = 0;

bool isNUM(char *str_input); 								// Checks if string is a number
string returnStringWithFitSize(int val, int stringMaxSize); // Adds 0 to the beginning of the string
void copy_lru_struct(LRU_struct &src, LRU_struct dest);

// file functions
void fillTheFile(FILE *file_input, int numberOfFill);		// Fills the file with 0
void updateDataInFile(FILE *file_input, int seek_input, string data); // Updates the disk's value of given index 

void fillVirtualMemSystem(FILE *file_input, int pageReplacement_type); // Assigns random values to all data on disk and fills virtual memory
void fillPyhsicalMem();

// sorting functions
void BubbleSort_thread(int *arr, int size);
void QuickSort_thread(int *arr, int lo, int hi);
int partition(int *arr, int lo, int hi);

void MergeSort_thread(int *arr, int left, int right);
void Merge(int arr[], int left, int mid, int right);

void IndexSort_thread(int *arr, int size);

void _set(unsigned int index, int value, char *tName);
int get(unsigned int index, char *tName);

//thread function
void *SortArr_Thread(void *index);

void printall_statistic();
void print_statistic(int index);
void printPageTable();

int main(int argc, char *argv[])
{

	if (argc != _INPUT_COUNT_)
	{
		cout << "Error!! Number of inputs must be " << _INPUT_COUNT_ << endl;
		exit(EXIT_FAILURE);
	}

	if (!isNUM(argv[1]) || !isNUM(argv[2]) || !isNUM(argv[3]) || isNUM(argv[4]) ||
		isNUM(argv[5]) || !isNUM(argv[6]) || isNUM(argv[7]))
	{
		cout << "Wrong user input" << endl
			 << "(1. ,2. ,3. and 6. input must be integer)" << endl;
		cout << "(4. ,5. 7. inputs must be string)" << endl;
		exit(EXIT_FAILURE);
	}

	int frameSize_i = atoi(argv[1]);
	int numPhysical_i = atoi(argv[2]);
	int numVirtual_i = atoi(argv[3]);
	pageTablePrintInt = atoi(argv[6]);

	if(frameSize_i <= 0 || numPhysical_i <= 0 || 
		numVirtual_i <= 0 || pageTablePrintInt <= 0)
	{
		cout<<"Your inputs can not be equal or less than 0"<<endl;
		exit(EXIT_FAILURE);
	}

	int thread_ids[4] = {0, 1, 2, 3};

	frameSize = pow(2, frameSize_i);
	numPhysicalFrames = pow(2, numPhysical_i);
	numVirtualFrames = pow(2, numVirtual_i);

	pageReplacement = argv[4];
	allocPolicy = argv[5];
	fileName = argv[7];

	if (strcmp(pageReplacement, "NRU") == 0)
		pageReplacement_type = 0;
	else if (strcmp(pageReplacement, "FIFO") == 0)
		pageReplacement_type = 1;
	else if (strcmp(pageReplacement, "SC") == 0)
		pageReplacement_type = 2;
	else if (strcmp(pageReplacement, "LRU") == 0)
		pageReplacement_type = 3;
	else if (strcmp(pageReplacement, "WSClock") == 0)
		pageReplacement_type = 4;
	else
	{
		cout << "UNKNOWN PAGE REPLACEMENT ALGORITHM" << endl;
		exit(EXIT_FAILURE);
	}

	srand(1000);

	VirtualMem_size = frameSize * numVirtualFrames;
	PhysicalMem_size = frameSize * numPhysicalFrames;

	if (PhysicalMem_size > VirtualMem_size)
	{
		cout << "Pysical memory size must not be bigger than Virtual memory size" << endl;
		exit(EXIT_FAILURE);
	}

	user_file = fopen(fileName, "w+");
	if (user_file == NULL)
	{
		cout << "File could not be created or opened" << endl;
		exit(EXIT_FAILURE);
	}
	ftruncate(fileno(user_file), 0); // reset the file

	fillTheFile(user_file, VirtualMem_size);

	PhysicalMem = (int *)malloc(sizeof(int) * PhysicalMem_size);
	virtualMem = (PageTableEntry **)malloc(sizeof(PageTableEntry *) * VirtualMem_size);
	int i;
	for (i = 0; i < VirtualMem_size; i++)
	{
		virtualMem[i] = (PageTableEntry *)malloc(sizeof(PageTableEntry) * 1);
	}

	fillPyhsicalMem();
	fillVirtualMemSystem(user_file, pageReplacement_type);

	pthread_mutex_init(&mutex_sort_thread, NULL);
	pthread_t sort_thread[4];

	for (i = 0; i < 4; i++)
	{
		if (pthread_create(&(sort_thread[i]), NULL, &SortArr_Thread, &(thread_ids[i])))
		{
			cout << "Error while creating thread" << endl;
			free(PhysicalMem);
			free(virtualMem);

			fclose(user_file);
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < 4; i++)
	{
		if (pthread_join((sort_thread[i]), NULL))
		{
			cout << "Error while waiting a thread" << endl;
			free(PhysicalMem);
			free(virtualMem);

			fclose(user_file);
			exit(EXIT_FAILURE);
		}
	}
	printall_statistic();

	free(PhysicalMem);
	free(virtualMem);

	fclose(user_file);
}

void fillTheFile(FILE *file_input, int numberOfFill)
{
	fseek(file_input, 0, SEEK_SET);
	string temp = "";
	int i;
	for (i = 0; i < MAX_INT_SIZE; i++)
		temp += "0";

	for (i = 0; i < numberOfFill; i++)
		fprintf(file_input, "%s\n", temp.c_str());
	fprintf(file_input, "\n");
	fseek(file_input, 0, SEEK_SET);
}

void fillPyhsicalMem()
{
	int i;
	for (i = 0; i < PhysicalMem_size; i++)
		PhysicalMem[i] = -1;
}

void BubbleSort_thread(int *arr, int size)
{
	int i, j, temp;
	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size - i - 1; j++)
		{
			if (arr[j] > arr[j + 1])
			{
				temp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = temp;
			}
		}
	}
}
void QuickSort_thread(int *arr, int lo, int hi)
{

	if (lo < hi)
	{
		int pivot = partition(arr, lo, hi);
		QuickSort_thread(arr, lo, pivot - 1);
		QuickSort_thread(arr, pivot + 1, hi);
	}
}

int partition(int *arr, int lo, int hi)
{
	int pivot = arr[hi];
	int i = (lo - 1);
	int j, temp;

	for (j = lo; j <= hi - 1; j++)
	{
		// If current element is smaller than or
		// equal to pivot
		if (arr[j] <= pivot)
		{
			i++; // increment index of smaller element
			temp = arr[i];
			arr[i] = arr[j];
			arr[j] = temp;
		}
	}
	temp = arr[i + 1];
	arr[i + 1] = arr[hi];
	arr[hi] = temp;

	return (i + 1);
}

void MergeSort_thread(int *arr, int left, int right)
{
	if (left < right)
	{
		int mid = left + (right - left) / 2;
		MergeSort_thread(arr, left, mid);
		MergeSort_thread(arr, mid + 1, right);
		Merge(arr, left, mid, right);
	}
}

void Merge(int arr[], int left, int mid, int right)
{
	int i, j, k;
	int firstHalf_size = mid - left + 1;
	int secondHalf_size = right - mid;

	int *firstHalf_arr = (int *)malloc(sizeof(int) * firstHalf_size);
	int *secondHalf_arr = (int *)malloc(sizeof(int) * secondHalf_size);

	for (i = 0; i < firstHalf_size; i++)
		firstHalf_arr[i] = arr[left + i];
	for (j = 0; j < secondHalf_size; j++)
		secondHalf_arr[j] = arr[mid + 1 + j];

	i = 0;
	j = 0;
	k = left;
	while (i < firstHalf_size && j < secondHalf_size)
	{
		if (firstHalf_arr[i] <= secondHalf_arr[j])
		{
			arr[k] = firstHalf_arr[i];
			i++;
		}
		else
		{
			arr[k] = secondHalf_arr[j];
			j++;
		}
		k++;
	}

	while (i < firstHalf_size)
	{
		arr[k] = firstHalf_arr[i];
		i++;
		k++;
	}

	while (j < secondHalf_size)
	{
		arr[k] = secondHalf_arr[j];
		j++;
		k++;
	}
	free(firstHalf_arr);
	free(secondHalf_arr);
}

void IndexSort_thread(int *arr, int size)
{
	int *indexes = (int *)malloc(sizeof(int) * size);
	int i, j, temp;
	for (i = 0; i < size; i++)
		indexes[i] = i;
	for (i = 0; i < size - 1; i++)
	{
		for (j = i + 1; j < size; j++)
		{
			if (arr[indexes[i]] > arr[indexes[j]])
			{
				temp = indexes[i];
				indexes[i] = indexes[j];
				indexes[j] = temp;
			}
		}
	}

	for (i = 0; i < size; i++)
	{
		arr[i] = arr[indexes[i]];
	}
}
void *SortArr_Thread(void *index)
{
	int thread_id = *((int *)index);
	int start_index, end_index;
	int quarter_virtual_size = VirtualMem_size / 4;
	int i;
	int *values = (int *)malloc(sizeof(int) * quarter_virtual_size);
	int current_index = 0;
	char temp_str[10];

	if (thread_id == 0) // Bubble Sort
	{
		start_index = 0;
		end_index = quarter_virtual_size;
		strcpy(temp_str, bubble_str);
		//tname = "bubble";
	}
	else if (thread_id == 1) // Quick Sort
	{
		start_index = quarter_virtual_size;
		end_index = 2 * quarter_virtual_size;
		strcpy(temp_str, quick_str);
		//tname = "quick";
	}
	else if (thread_id == 2) // Merge Sort
	{
		start_index = 2 * quarter_virtual_size;
		end_index = 3 * quarter_virtual_size;
		strcpy(temp_str, merge_str);
		//tname = "merge";
	}
	else // Index Sort
	{
		start_index = 3 * quarter_virtual_size;
		end_index = VirtualMem_size;
		strcpy(temp_str, index_str);
		//tname = "index";
	}

	current_index = 0;
	for (i = start_index; i < end_index; i++) // each sort algorithm retrieves data in its own quarter in virtual memory with get
	{
		pthread_mutex_lock(&mutex_sort_thread);
		values[current_index] = get(i, temp_str); // get all values and assign it values arr
		current_index++;
		pthread_mutex_unlock(&mutex_sort_thread);
	}

	if (thread_id == 0) // Bubble Sort
	{
		BubbleSort_thread(values, quarter_virtual_size);
	}
	else if (thread_id == 1) // Quick Sort
	{
		QuickSort_thread(values, 0, (quarter_virtual_size - 1));
	}
	else if (thread_id == 2) // Merge Sort
	{
		MergeSort_thread(values, 0, (quarter_virtual_size - 1));
	}
	else // Index Sort
	{
		//MergeSort_thread(values, 0, (quarter_virtual_size - 1));

		IndexSort_thread(values, quarter_virtual_size);
	}

	current_index = 0;
	string sendToFile;

	//strcpy(temp_str, check_str);
	// Check part
	// Each thread will check its own part of array
	for (i = start_index; i < end_index; i++)
	{
		// _set virtualMem[i] = values[current_index];
		pthread_mutex_lock(&mutex_sort_thread);

		//_set(i, values[current_index], temp_str);

		current_index++;
		pthread_mutex_unlock(&mutex_sort_thread);

		// check part, check the all values and make sure it is sorted
		if ((current_index + 1 != quarter_virtual_size) && (values[current_index] > values[current_index + 1]))
		{	//if it is not sorted give error message
			if (thread_id == 0) // Bubble Sort
			{
				cout << "Bubble sort part is unsorted!!" << endl;
				free(values);
				return NULL;
			}
			else if (thread_id == 1) // Quick Sort
			{
				cout << "Quick sort part is unsorted!!" << endl;
				free(values);
				return NULL;
			}
			else if (thread_id == 2) // Merge Sort
			{
				cout << "Merge sort part is unsorted!!" << endl;
				free(values);
				return NULL;
			}
			else // Index Sort
			{
				//cout << "Index sort part is unsorted!!" << endl;
			}
			
		}
		statistic_arr[5][0] += 1;
		//current_index++;
	}

	if (thread_id == 0) // Bubble Sort
	{
		cout << "Bubble sort part checked" << endl;
	}
	else if (thread_id == 1) // Quick Sort
	{
		cout << "Quick sort part checked" << endl;
	}
	else if (thread_id == 2) // Merge Sort
	{
		cout << "Merge sort part checked" << endl;
	}
	else // Index Sort
	{
		cout << "Index sort part checked" << endl;
	}

	free(values);
	return NULL;
}

void fillVirtualMemSystem(FILE *file_input, int pageReplacement_type)
{
	int i, j;
	
	int isUsed = 0;
	int rand_num_toPhysical, rand_val;
	string sendToFile = "";
	int seek_file;

	i = 0;
	while (i < PhysicalMem_size) // this loop fills pysical memory and virtual memory
	{
		
		rand_num_toPhysical = i;
		isUsed = 0;
		
		








		if (isUsed == 0) // if random val is not used
		{
			//usedIndexes.push_back(rand_num_toPhysical);

			//virtualMem[i]->CachingDisabled = -1;
			virtualMem[i]->isModified = false;
			virtualMem[i]->isPresent = 1;
			virtualMem[i]->isReferenced = false;
			//virtualMem[i]->Protection = "null";
			virtualMem[i]->toPhysical_index = rand_num_toPhysical;
			rand_val = rand();
			//virtualMem[i]->val = rand_val;
			PhysicalMem[i] = rand_val;

			if (pageReplacement_type == 0) // NRU
			{
				nru_wsc_vector.push_back(i);
			}
			else if (pageReplacement_type == 1) // FIFO
			{
				// push the value which is the index of virtual memory that links pyhsical memory
				fifo_queue.push(i);
			}
			else if (pageReplacement_type == 2) // SC
			{
				// push the value which is the index of virtual memory that links pyhsical memory
				fifo_queue.push(i);
			}
			else if (pageReplacement_type == 3) // LRU
			{
				LRU_struct temp_lru;
				time_t t = time(NULL);
				struct tm *timePtr = localtime(&t);

				temp_lru.day = timePtr->tm_mday;
				temp_lru.hour = timePtr->tm_hour;
				temp_lru.minute = timePtr->tm_min;
				temp_lru.second = timePtr->tm_sec;

				temp_lru.id = i;

				LRU_vector.push_back(temp_lru);
			}
			else if (pageReplacement_type == 4) // WSClock
			{
				nru_wsc_vector.push_back(i);
			}

			// update the value in disk given index
			sendToFile = returnStringWithFitSize(rand_val, MAX_INT_SIZE);
			seek_file = (i * MAX_INT_SIZE) + i;
			updateDataInFile(file_input, seek_file, sendToFile);
			i++;
		}

		statistic_arr[0][0]++; // Number of reads
		statistic_arr[0][1]++; // Number of writes
		statistic_arr[0][4]++; // Number of disk page writes
		statistic_arr[0][5]++; // Number of disk page reads
	}

	for (j = i; j < VirtualMem_size; j++) //this loop fills places that are not filled in virtual memory
	{
		rand_val = rand();
		//virtualMem[j]->CachingDisabled = -1;
		virtualMem[j]->isModified = false;
		virtualMem[j]->isPresent = 0;
		virtualMem[j]->isReferenced = false;
		//virtualMem[j]->Protection = "null";
		virtualMem[j]->toPhysical_index = -1;

		// set the random value to the disk with given index
		sendToFile = returnStringWithFitSize(rand_val, MAX_INT_SIZE);
		seek_file = (j * MAX_INT_SIZE) + j;
		updateDataInFile(file_input, seek_file, sendToFile);
		statistic_arr[0][5]++;
	}
}

int get(unsigned int index, char *tName)
{

	// it will just return the value from the disk
	// This will be used only for filling virtual memory
	// and checking the sorted file for controlling that it is true
	if (strcmp(tName, "fill") == 0 || strcmp(tName, "check") == 0)
	{
		int disk_val;
		fseek(user_file, ((index * MAX_INT_SIZE) + index), SEEK_SET);
		fscanf(user_file, "%d", &disk_val);

		memoryaccess_counter++;
		if(memoryaccess_counter >= pageTablePrintInt)
		{
			memoryaccess_counter=0;
			printPageTable();
		}

		return disk_val;
	}

	// for all sorting algorithm, same get algorithm will work
	// there is no need to implement different algorithm for it...
	else
	{
		int quick_index = 1, bubble_index = 2, merge_index = 3, index_index = 4;

		if (virtualMem[index]->isPresent == 0) // page replacement
		{
			int fseek_set = (MAX_INT_SIZE * index) + index;
			int disk_val;
			fseek(user_file, fseek_set, SEEK_SET);
			char *disk_str = (char *)malloc(sizeof(char) * MAX_INT_SIZE);
			fscanf(user_file, "%s", disk_str);
			disk_val = atoi(disk_str);
			string sendToFile;

			if (strcmp(tName, "quick") == 0)
			{
				statistic_arr[quick_index][2]++;
				statistic_arr[quick_index][5]++;
				statistic_arr[quick_index][3]++;
			}
			else if (strcmp(tName, "bubble") == 0)
			{
				statistic_arr[bubble_index][2]++;
				statistic_arr[bubble_index][5]++;
				statistic_arr[bubble_index][3]++;
			}

			else if (strcmp(tName, "merge") == 0)
			{
				statistic_arr[merge_index][2]++;
				statistic_arr[merge_index][5]++;
				statistic_arr[merge_index][3]++;
			}
			else if (strcmp(tName, "index") == 0)
			{
				statistic_arr[index_index][2]++;
				statistic_arr[index_index][5]++;
				statistic_arr[index_index][3]++;
			}

			int virtual_index;
			int toPhysical_index;

			if (pageReplacement_type == 0) // NRU
			{
				int i;
				int nru_wsc_vector_size = nru_wsc_vector.size();
				int stage = 0;
				int control = 0;
				int numberofreads = 0;
				while (control == 0)
				{

					for (i = 0; i < nru_wsc_vector_size; i++)
					{
						numberofreads++;
						virtual_index = nru_wsc_vector.at(i);

						if (stage == 0)
						{
							if (virtualMem[virtual_index]->isReferenced == false &&
								virtualMem[virtual_index]->isModified == false)
							{

								toPhysical_index = virtualMem[virtual_index]->toPhysical_index;
								//virtualMem[index]->val = disk_val;
								virtualMem[index]->isPresent = 1;
								virtualMem[index]->toPhysical_index = toPhysical_index;

								virtualMem[virtual_index]->isPresent = 0;
								virtualMem[virtual_index]->toPhysical_index = -1;

								// backup the value of old physical value
								sendToFile = returnStringWithFitSize(PhysicalMem[toPhysical_index], MAX_INT_SIZE);
								updateDataInFile(user_file, ((virtual_index * MAX_INT_SIZE) + virtual_index), sendToFile);

								PhysicalMem[toPhysical_index] = disk_val;

								nru_wsc_vector.erase(nru_wsc_vector.begin() + i);
								nru_wsc_vector.push_back(index);
								control = 1;
								break;
							}
						}
						else if (stage == 1)
						{
							if (virtualMem[virtual_index]->isReferenced == false &&
								virtualMem[virtual_index]->isModified == true)
							{
								toPhysical_index = virtualMem[virtual_index]->toPhysical_index;
								//virtualMem[index]->val = disk_val;
								virtualMem[index]->isPresent = 1;
								virtualMem[index]->toPhysical_index = toPhysical_index;

								virtualMem[virtual_index]->isPresent = 0;
								virtualMem[virtual_index]->toPhysical_index = -1;

								// backup the value of old physical value
								sendToFile = returnStringWithFitSize(PhysicalMem[toPhysical_index], MAX_INT_SIZE);
								updateDataInFile(user_file, ((virtual_index * MAX_INT_SIZE) + virtual_index), sendToFile);

								PhysicalMem[toPhysical_index] = disk_val;

								nru_wsc_vector.erase(nru_wsc_vector.begin() + i);
								nru_wsc_vector.push_back(index);
								control = 1;
								break;
							}
						}

						else if (stage == 2)
						{

							if (virtualMem[virtual_index]->isReferenced == true &&
								virtualMem[virtual_index]->isModified == false)
							{
								toPhysical_index = virtualMem[virtual_index]->toPhysical_index;
								//virtualMem[index]->val = disk_val;
								virtualMem[index]->isPresent = 1;
								virtualMem[index]->toPhysical_index = toPhysical_index;

								virtualMem[virtual_index]->isPresent = 0;
								virtualMem[virtual_index]->toPhysical_index = -1;

								// backup the value of old physical value
								sendToFile = returnStringWithFitSize(PhysicalMem[toPhysical_index], MAX_INT_SIZE);
								updateDataInFile(user_file, ((virtual_index * MAX_INT_SIZE) + virtual_index), sendToFile);

								PhysicalMem[toPhysical_index] = disk_val;

								nru_wsc_vector.erase(nru_wsc_vector.begin() + i);
								nru_wsc_vector.push_back(index);
								control = 1;
								break;
							}
						}

						else if (stage == 3)
						{
							if (virtualMem[virtual_index]->isReferenced == true &&
								virtualMem[virtual_index]->isModified == true)
							{
								toPhysical_index = virtualMem[virtual_index]->toPhysical_index;
								//virtualMem[index]->val = disk_val;
								virtualMem[index]->isPresent = 1;
								virtualMem[index]->toPhysical_index = toPhysical_index;

								virtualMem[virtual_index]->isPresent = 0;
								virtualMem[virtual_index]->toPhysical_index = -1;

								// backup the value of old physical value
								sendToFile = returnStringWithFitSize(PhysicalMem[toPhysical_index], MAX_INT_SIZE);
								updateDataInFile(user_file, ((virtual_index * MAX_INT_SIZE) + virtual_index), sendToFile);

								PhysicalMem[toPhysical_index] = disk_val;

								nru_wsc_vector.erase(nru_wsc_vector.begin() + i);
								nru_wsc_vector.push_back(index);
								control = 1;
								break;
							}
						}

						else
						{
							cout << "Something went wrong in get function (stage can not be bigger than 3)" << endl;
							free(disk_str);
							exit(EXIT_FAILURE);
						}
					}

					if (control == 1)
					{
						if (strcmp(tName, "quick") == 0)
						{
							statistic_arr[quick_index][0] += numberofreads;
							statistic_arr[quick_index][1]++;
							statistic_arr[quick_index][4]++;
						}
						else if (strcmp(tName, "bubble") == 0)
						{
							statistic_arr[bubble_index][0] += numberofreads;
							statistic_arr[bubble_index][1]++;
							statistic_arr[bubble_index][4]++;
						}

						else if (strcmp(tName, "merge") == 0)
						{
							statistic_arr[merge_index][0] += numberofreads;
							statistic_arr[merge_index][1]++;
							statistic_arr[merge_index][4]++;
						}
						else if (strcmp(tName, "index") == 0)
						{
							statistic_arr[index_index][0] += numberofreads;
							statistic_arr[index_index][1]++;
							statistic_arr[index_index][4]++;
						}
						break;
					}

					stage++; // look for another class for R and M bits
				}
			}
			else if (pageReplacement_type == 1) // FIFO
			{
				// get first value from fifo queue
				virtual_index = fifo_queue.front(); // get first element in queue
				toPhysical_index = virtualMem[virtual_index]->toPhysical_index;
				//virtualMem[index]->val = disk_val;
				virtualMem[index]->isPresent = 1;
				virtualMem[index]->toPhysical_index = toPhysical_index;

				virtualMem[virtual_index]->isPresent = 0;
				virtualMem[virtual_index]->toPhysical_index = -1;

				// backup the value of old physical value
				sendToFile = returnStringWithFitSize(PhysicalMem[toPhysical_index], MAX_INT_SIZE);
				updateDataInFile(user_file, ((virtual_index * MAX_INT_SIZE) + virtual_index), sendToFile);

				PhysicalMem[toPhysical_index] = disk_val;
				fifo_queue.pop(); // remove first element
				fifo_queue.push(index); // add new page entry index

				if (strcmp(tName, "quick") == 0)
				{
					statistic_arr[quick_index][0]++;
					statistic_arr[quick_index][1]++;
					statistic_arr[quick_index][4]++;
				}
				else if (strcmp(tName, "bubble") == 0)
				{
					statistic_arr[bubble_index][0]++;
					statistic_arr[bubble_index][1]++;
					statistic_arr[bubble_index][4]++;
				}

				else if (strcmp(tName, "merge") == 0)
				{
					statistic_arr[merge_index][0]++;
					statistic_arr[merge_index][1]++;
					statistic_arr[merge_index][4]++;
				}
				else if (strcmp(tName, "index") == 0)
				{
					statistic_arr[index_index][0]++;
					statistic_arr[index_index][1]++;
					statistic_arr[index_index][4]++;
				}
			}
			else if (pageReplacement_type == 2) // SC
			{
				bool isFound = false;
				int numberofread = 0;
				while (isFound == false) // Continue until you find the page entr with R bit 0
				{
					numberofread++;
					virtual_index = fifo_queue.front();
					if (virtualMem[index]->isReferenced == false) // R == 0, if R bit is 0 do page replacement
					{
						toPhysical_index = virtualMem[virtual_index]->toPhysical_index;
						//virtualMem[index]->val = disk_val;
						virtualMem[index]->isPresent = 1;
						virtualMem[index]->toPhysical_index = toPhysical_index;

						virtualMem[virtual_index]->isPresent = 0;
						virtualMem[virtual_index]->toPhysical_index = -1;

						// backup the value of old physical value
						sendToFile = returnStringWithFitSize(PhysicalMem[toPhysical_index], MAX_INT_SIZE);
						updateDataInFile(user_file, ((virtual_index * MAX_INT_SIZE) + virtual_index), sendToFile);

						PhysicalMem[toPhysical_index] = disk_val;
						fifo_queue.pop();
						fifo_queue.push(index);

						isFound = true;
					}
					else // R == 1 -> give second chance
					{
						// Make R bit of page entry 0
						virtualMem[virtual_index]->isReferenced = false; // clear reference bit

						fifo_queue.pop(); // remove from queue and add again
						fifo_queue.push(virtual_index); // move page to tail of FIFO list
														// isFound = false so it will check next-oldest page
					}
				}
				if (strcmp(tName, "quick") == 0)
				{
					statistic_arr[quick_index][0] += numberofread;
					statistic_arr[quick_index][1]++;
					statistic_arr[quick_index][4]++;
				}
				else if (strcmp(tName, "bubble") == 0)
				{
					statistic_arr[bubble_index][0] += numberofread;
					statistic_arr[bubble_index][1]++;
					statistic_arr[bubble_index][4]++;
				}

				else if (strcmp(tName, "merge") == 0)
				{
					statistic_arr[merge_index][0] += numberofread;
					statistic_arr[merge_index][1]++;
					statistic_arr[merge_index][4]++;
				}
				else if (strcmp(tName, "index") == 0)
				{
					statistic_arr[index_index][0] += numberofread;
					statistic_arr[index_index][1]++;
					statistic_arr[index_index][4]++;
				}
			}
			else if (pageReplacement_type == 3) // LRU
			{
				int i;
				int lru_vector_size = LRU_vector.size();
				int minIndex_LRU = -1;
				int numberOfRead = 0;
				LRU_struct min_lru_entry, temp_lru_entry;

				min_lru_entry.day = 61;
				min_lru_entry.hour = 61;
				min_lru_entry.minute = 61;
				min_lru_entry.second = 61;
				min_lru_entry.id = -1;

				for (i = 0; i < lru_vector_size; i++) // find least reacently used entry and do page replacement
				{
					numberOfRead++;
					copy_lru_struct(temp_lru_entry, LRU_vector.at(i));

					if (min_lru_entry.day > temp_lru_entry.day)
					{
						min_lru_entry.day = temp_lru_entry.day;
						min_lru_entry.hour = temp_lru_entry.hour;
						min_lru_entry.minute = temp_lru_entry.minute;
						min_lru_entry.second = temp_lru_entry.second;
						min_lru_entry.id = temp_lru_entry.id;
						minIndex_LRU = i;
					}
					else if (min_lru_entry.day == temp_lru_entry.day)
					{
						if (min_lru_entry.hour > temp_lru_entry.hour)
						{
							min_lru_entry.day = temp_lru_entry.day;
							min_lru_entry.hour = temp_lru_entry.hour;
							min_lru_entry.minute = temp_lru_entry.minute;
							min_lru_entry.second = temp_lru_entry.second;
							min_lru_entry.id = temp_lru_entry.id;
							minIndex_LRU = i;
						}

						else if (min_lru_entry.hour == temp_lru_entry.hour)
						{
							if (min_lru_entry.minute > temp_lru_entry.minute)
							{
								min_lru_entry.day = temp_lru_entry.day;
								min_lru_entry.hour = temp_lru_entry.hour;
								min_lru_entry.minute = temp_lru_entry.minute;
								min_lru_entry.second = temp_lru_entry.second;
								min_lru_entry.id = temp_lru_entry.id;
								minIndex_LRU = i;
							}

							else if (min_lru_entry.minute == temp_lru_entry.minute)
							{
								if (min_lru_entry.second > temp_lru_entry.second)
								{
									min_lru_entry.day = temp_lru_entry.day;
									min_lru_entry.hour = temp_lru_entry.hour;
									min_lru_entry.minute = temp_lru_entry.minute;
									min_lru_entry.second = temp_lru_entry.second;
									min_lru_entry.id = temp_lru_entry.id;
									minIndex_LRU = i;
								}
							}
						}
					}
				}

				virtual_index = min_lru_entry.id;
				toPhysical_index = virtualMem[virtual_index]->toPhysical_index;
				//virtualMem[index]->val = disk_val;
				virtualMem[index]->isPresent = 1;
				virtualMem[index]->toPhysical_index = toPhysical_index;

				virtualMem[virtual_index]->isPresent = 0;
				virtualMem[virtual_index]->toPhysical_index = -1;

				// backup the value of old physical value
				sendToFile = returnStringWithFitSize(PhysicalMem[toPhysical_index], MAX_INT_SIZE);
				updateDataInFile(user_file, ((virtual_index * MAX_INT_SIZE) + virtual_index), sendToFile);

				PhysicalMem[toPhysical_index] = disk_val;

				LRU_vector.erase(LRU_vector.begin() + minIndex_LRU);

				LRU_struct temp_lru_send;
				time_t t = time(NULL);
				struct tm *timePtr = localtime(&t); // get time for new entry

				temp_lru_send.day = timePtr->tm_mday;
				temp_lru_send.hour = timePtr->tm_hour;
				temp_lru_send.minute = timePtr->tm_min;
				temp_lru_send.second = timePtr->tm_sec;
				temp_lru_send.id = index;

				LRU_vector.push_back(temp_lru_send); // push new value to the lru table

				if (strcmp(tName, "quick") == 0)
				{
					statistic_arr[quick_index][0] += numberOfRead;
					statistic_arr[quick_index][1]++;
					statistic_arr[quick_index][4]++;
				}
				else if (strcmp(tName, "bubble") == 0)
				{
					statistic_arr[bubble_index][0] += numberOfRead;
					statistic_arr[bubble_index][1]++;
					statistic_arr[bubble_index][4]++;
				}

				else if (strcmp(tName, "merge") == 0)
				{
					statistic_arr[merge_index][0] += numberOfRead;
					statistic_arr[merge_index][1]++;
					statistic_arr[merge_index][4]++;
				}
				else if (strcmp(tName, "index") == 0)
				{
					statistic_arr[index_index][0] += numberOfRead;
					statistic_arr[index_index][1]++;
					statistic_arr[index_index][4]++;
				}
			}
			else if (pageReplacement_type == 4) // WSClock
			{
				int vector_size = nru_wsc_vector.size();
				int control = 0;
				int numberOfRead = 0;
				while (control == 0)
				{
					numberOfRead++;
					virtual_index = nru_wsc_vector.at(WSClock_currentpoint);

					// if R=0 replace
					if (virtualMem[virtual_index]->isReferenced == false)
					{

						toPhysical_index = virtualMem[virtual_index]->toPhysical_index;
						//virtualMem[index]->val = disk_val;
						virtualMem[index]->isPresent = 1;
						virtualMem[index]->toPhysical_index = toPhysical_index;

						virtualMem[virtual_index]->isPresent = 0;
						virtualMem[virtual_index]->toPhysical_index = -1;

						// backup the value of old physical value
						sendToFile = returnStringWithFitSize(PhysicalMem[toPhysical_index], MAX_INT_SIZE);
						updateDataInFile(user_file, ((virtual_index * MAX_INT_SIZE) + virtual_index), sendToFile);

						PhysicalMem[toPhysical_index] = disk_val;

						nru_wsc_vector[WSClock_currentpoint] = index; // change clock's current value with new page entry
						control = 1;
						WSClock_currentpoint++;
						if (WSClock_currentpoint >= vector_size)
							WSClock_currentpoint = 0;
					}

					// if R=0 replace _set R->0 and go to next clock
					else
					{
						virtualMem[virtual_index]->isReferenced = 0;
						WSClock_currentpoint = (WSClock_currentpoint + 1);
						if (WSClock_currentpoint >= vector_size)
							WSClock_currentpoint = 0;
					}
				}

				if (strcmp(tName, "quick") == 0)
				{
					statistic_arr[quick_index][0] += numberOfRead;
					statistic_arr[quick_index][1]++;
					statistic_arr[quick_index][4]++;
				}
				else if (strcmp(tName, "bubble") == 0)
				{
					statistic_arr[bubble_index][0] += numberOfRead;
					statistic_arr[bubble_index][1]++;
					statistic_arr[bubble_index][4]++;
				}

				else if (strcmp(tName, "merge") == 0)
				{
					statistic_arr[merge_index][0] += numberOfRead;
					statistic_arr[merge_index][1]++;
					statistic_arr[merge_index][4]++;
				}
				else if (strcmp(tName, "index") == 0)
				{
					statistic_arr[index_index][0] += numberOfRead;
					statistic_arr[index_index][1]++;
					statistic_arr[index_index][4]++;
				}
			}

			memoryaccess_counter++;
			if(memoryaccess_counter >= pageTablePrintInt)
			{
				memoryaccess_counter=0;
				printPageTable();
			}
			
			free(disk_str);
			return disk_val;
		}
		else // if searching index is pointed with pyhsical memory, do not make page replacement, return value in physical memory 
		{
			if (strcmp(tName, "quick") == 0)
			{
				statistic_arr[quick_index][0]++;
			}
			else if (strcmp(tName, "bubble") == 0)
			{
				statistic_arr[bubble_index][0]++;
			}

			else if (strcmp(tName, "merge") == 0)
			{
				statistic_arr[merge_index][0]++;
			}
			else if (strcmp(tName, "index") == 0)
			{
				statistic_arr[index_index][0]++;
			}
			
			memoryaccess_counter++;
			if(memoryaccess_counter >= pageTablePrintInt)
			{
				memoryaccess_counter=0;
				printPageTable();
			}

			return (PhysicalMem[virtualMem[index]->toPhysical_index]);
		}
	}
}

void _set(unsigned int index, int value, char *tName)
{

	if ((int)index >= VirtualMem_size)
		return;

	int fseek_set = (index * MAX_INT_SIZE) + index;
	string sendToFile = returnStringWithFitSize(value, MAX_INT_SIZE);
	updateDataInFile(user_file, fseek_set, sendToFile);

	int fillindex = 0, quick_index = 1, bubble_index = 2, merge_index = 3, index_index = 4, checkindex = 5;

	if (strcmp(tName, "fill") == 0)
	{
		statistic_arr[fillindex][4]++;
		statistic_arr[fillindex][1]++;
	}

	else if (strcmp(tName, "quick") == 0)
	{
		statistic_arr[quick_index][4]++;
		statistic_arr[quick_index][1]++;
	}
	else if (strcmp(tName, "bubble") == 0)
	{
		statistic_arr[bubble_index][4]++;
		statistic_arr[bubble_index][1]++;
	}

	else if (strcmp(tName, "merge") == 0)
	{
		statistic_arr[merge_index][4]++;
		statistic_arr[merge_index][1]++;
	}
	else if (strcmp(tName, "index") == 0)
	{
		statistic_arr[index_index][4]++;
		statistic_arr[index_index][1]++;
	}
	else
	{
		statistic_arr[checkindex][4]++;
		statistic_arr[checkindex][1]++;
		
	}

	memoryaccess_counter++;
	if(memoryaccess_counter >= pageTablePrintInt)
	{
		memoryaccess_counter=0;
		printPageTable();
	}

}

void printPageTable()
{
	string temp=" ";
	int i;
	cout<<endl<<"-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-"<<endl;

	printf("%10sindex %5sreferenced %7smodified %8spresent toPyhsicalIndex\n",
		temp.c_str(),temp.c_str(),temp.c_str(),temp.c_str());
	for(i=0; i<VirtualMem_size; i++)
	{
		printf("%15d %15d %15d %15d %15d\n",
		i,virtualMem[i]->isReferenced,virtualMem[i]->isModified,
		virtualMem[i]->isPresent, virtualMem[i]->toPhysical_index);
	}
	printf("%10sindex %5sreferenced %7smodified %8spresent toPyhsicalIndex\n",
		temp.c_str(),temp.c_str(),temp.c_str(),temp.c_str());
	cout<<"-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-"<<endl<<endl;
}

string returnStringWithFitSize(int val, int stringMaxSize)
{
	string result = to_string(val);
	int numberOfDigit = result.length();
	string tempZero = "";

	if (stringMaxSize < numberOfDigit)
		return NULL;

	int i;
	for (i = 0; i < (stringMaxSize - numberOfDigit); i++)
		tempZero += "0";

	tempZero += result;
	return tempZero;
}

void updateDataInFile(FILE *file_input, int seek_input, string data)
{
	fseek(file_input, seek_input, SEEK_SET);
	fprintf(file_input, "%s", data.c_str());
	fseek(file_input, 0, SEEK_SET);
}

bool isNUM(char *str_input)
{
	if (str_input == NULL)
		return false;
	int length = strlen(str_input);
	int i = 0;
	for (i = 0; i < length; i++)
	{
		if (!(str_input[i] >= '0' && str_input[i] <= '9'))
			return false;
	}
	return true;
}

void copy_lru_struct(LRU_struct &src, LRU_struct dest)
{
	src.day = dest.day;
	src.hour = dest.hour;
	src.minute = dest.minute;
	src.second = dest.second;
	src.id = dest.id;
}

void printall_statistic()
{
	cout << "-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-" << endl;

	cout << "fill statistics" << endl;
	print_statistic(0);
	cout << "quick statistics" << endl;
	print_statistic(1);
	cout << "bubble statistics" << endl;
	print_statistic(2);
	cout << "merge statistics" << endl;
	print_statistic(3);
	cout << "index statistics" << endl;
	print_statistic(4);
	cout << "check statistics" << endl;
	print_statistic(5);

	cout << "-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-" << endl;
}

void print_statistic(int index)
{
	cout << "number of reads : " << statistic_arr[index][0] << endl;
	cout << "number of write : " << statistic_arr[index][1] << endl;
	cout << "number of page misses : " << statistic_arr[index][2] << endl;
	cout << "number of page replacements : " << statistic_arr[index][3] << endl;
	cout << "number of disk page writes : " << statistic_arr[index][4] << endl;
	cout << "number of disk page read : " << statistic_arr[index][5] << endl;
	cout << endl
		 << endl;
}
