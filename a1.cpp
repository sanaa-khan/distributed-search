#include <stdio.h>
#include <omp.h>
#include <mpi.h>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <iostream>

using namespace std;

void printArray(int arr[], int num) {
    for (int i = 0; i < num; i++)
        cout << arr[i] << " ";
}

int main(int argc, char** argv) {

    srand(time(NULL));

	int rank, nprocs;
    const int num_each = 4;
    int to_send[num_each];
	int count = 0;

    // initialising MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
    // master code
	if (rank == 0) {
        cout << "There are " << nprocs - 1 << "  slave processes, each will have " << num_each << " numbers\n";

        // initialising list of numbers
        int list_size = num_each * (nprocs - 1);
        int* list = new int [list_size];

        for (int i = 0; i <  list_size; i++) {
            list[i] = rand() % 100;
        }

        cout << "\nProcess of rank 0 has input data: ";
        printArray(list, list_size);

        int to_find;
        cout << "\nEnter number to search for: ";
        cin >> to_find;

        // distributing list among slaves
        for (int i = 1, k = 0; i < nprocs; i++, k += num_each) {
            // each slave has to get "num_each" numbers, so we jump the array address forward for every iteration
            // e.g. slave 1 gets first 2 elements, slave 2 gets next 2 elements and so on
            MPI_Send(list + k, num_each, MPI_INT, i, 100, MPI_COMM_WORLD);

            // sending the number to search for (to all slaves)
            MPI_Send(&to_find, 1, MPI_INT, i, 101, MPI_COMM_WORLD);
        }

        bool flag = true;
        int notfound_counter = 0;

        // master communicating with slaves
        while (flag) {
            int buf = 0;
            MPI_Status status;

            for (int i = 1; i < nprocs; i++) {
                // master signals all slaves to search
                MPI_Send(&buf, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

                // recieves signal from slaves
                MPI_Recv(&buf , 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                // tag of 2 indicates a slave has found the number
                if (status.MPI_TAG == 2) {
                    cout << "\nMaster: Process " << status.MPI_SOURCE << " has found the number." << endl;
                    cout << "Master: Signaling all processes to abort!" << endl << endl;

                    // informing all slaves to abort
                    for (int j = 1; j < nprocs; j++) {
                            MPI_Send(&buf, 1, MPI_INT, j, 1, MPI_COMM_WORLD);
                    }

                    // for exiting loop and letting master terminate
                    flag = false;
                    break;
                }

                // a slave has finished searching and hasn't found the number
                else if (status.MPI_TAG == 4) {
                    notfound_counter++;

                    if (notfound_counter == 5) {
                        cout << "\n\nMaster: All processes have finished searching and have not found the given number in the array." << endl;
                        flag = false;
                    }
                }
            }
        }

        sleep(2);
        cout << "------------------------------------------\nSana Ali Khan \n18i-0439\n------------------------------------------" << endl;
	}

    else {
        int for_recv[num_each], to_find;
        bool flag = false, abort_flag = false;

        // slaves recieve their assigned numbers and the number to search for
        MPI_Recv(for_recv, num_each, MPI_INT, 0, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&to_find, 1, MPI_INT, 0, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        #pragma omp critical
        {
            // display local data
            cout << "\nProcess of rank " << rank << " has input data: ";
            printArray(for_recv, num_each);
        }

        //cout << "Number to find : " << to_find << endl;

        #pragma omp barrier
        {
            // iterate through array
            for (int i = 0; i < num_each; i++) {
                int buf;
                MPI_Status status;

                // recieve signal from master - continue or abort
                MPI_Recv(&buf, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                // signal to continue search
                if (status.MPI_TAG == 0) {
                    if (for_recv[i] == to_find) {
                        cout << "\n\nProcess " << rank << ": Number found at index " << i << endl;

                        // indicates that this slave did manage to find the number
                        flag = true;

                        // signal master that number has been found
                        MPI_Send(&buf, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
                    }

                    else {
                        // signal master that number has not been found (yet)
                        MPI_Send(&buf, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
                    }
                }

                // signal to terminate search and abort
                else if (status.MPI_TAG == 1) {
                    #pragma omp critical
                    {
                        cout << "Process " << rank << ": Aborting search!" << endl;

                        abort_flag = true;

                        i = num_each;
                    }
                }
            }

            // in case slave completed search but found nothing
            if (flag == false and abort_flag == false)
                cout << "\nProcess " << rank << ": Finished search, number not found.";

                // signal master that number was not found
                int buf;
                MPI_Send(&buf, 1, MPI_INT, 0, 4, MPI_COMM_WORLD);
        }
    }
	
	MPI_Finalize();
	
	return 0;
}
