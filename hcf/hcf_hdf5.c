
/*  
 *  This example writes data to the HDF5 file.
 *  Data conversion is performed during write operation.  
 */
 
#include <stddef.h>
#include <hdf5.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE "hdf5.h5"
#define DATASETNAME "IntArray" 
#define NX 4096                    /* dataset dimensions */
#define NY 4096
#define RANK 2


//mew stuff

int update_hlim_value(int row, int col, int hlim)
{
    hid_t file, dataset;
    hid_t datatype, dataspace;
    herr_t status;

    int * data; 
    printf("check 1\n");
    /* dynamically allocating memory to the matrix*/
    data = (int *)malloc(16777216 * sizeof(int));
    // for(int i=0; i<NX; i++)
    // {
    //     data[i] = (int *)malloc(NY * sizeof(int));
    // }
    printf("check 2\n");

    file=H5Fopen(FILE, H5F_ACC_RDWR, H5P_DEFAULT);
    printf("check 3\n");
    dataset=H5Dopen2(file, DATASETNAME,H5P_DEFAULT);
    printf("check 4\n");
    status = H5Dread(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    printf("check 5\n");
    int test = data[row * 4096 + col];
    printf("%d\n",test);
    //data[row][col] = hlim;
    data[row * 4096 + col]=hlim;
    printf("check 6\n");
    status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    printf("check 7\n");
    H5Dclose(dataset);
    H5Fclose(file);
    printf("check 8\n");
    // for (int i=0;i<NX;i++)
    // {
    //     free(data[i]);
    // }
    printf("check 9\n");
    free(data);
    return 0;

}

//end of new stuff

// int main()
int main_()
{
    hid_t file, dataset;         /* file and dataset handles */
    hid_t datatype, dataspace;   /* handles */
    hsize_t dimsf[2];              /* dataset dimensions */
    herr_t status;                             
    
    int * data;          /* data to write */
    int i, j;

    /* dynamically allocating memory to the matrix*/
    // data = (int **)malloc(NX * sizeof(int *));
    // for(i=0; i<NX; i++)
    // {
        // data[i] = (int *)malloc(NY * sizeof(int));
    // }
    data = (int *)malloc(16777216*sizeof(int));

    /* 
     * Data  and output buffer initialization. 
     */
    // for(i=0; i<NX; i++)
    // {
    //     for(j=0; j<NY; j++)
    //     {
    //         data[i][j] = -1;
    //     }
    // }

    memset(data, -1, NX * NY * sizeof(int));

    /*
     * 0 1 2 3 4 5 
     * 1 2 3 4 5 6
     * 2 3 4 5 6 7
     * 3 4 5 6 7 8
     * 4 5 6 7 8 9
     */

    /*
     * Create a new file using H5F_ACC_TRUNC access,
     * default file creation properties, and default file
     * access properties.
     */
    file = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Describe the size of the array and create the data space for fixed
     * size dataset. 
     */
    dimsf[0] = NX;
    dimsf[1] = NY;
    dataspace = H5Screate_simple(RANK, dimsf, NULL); 

    /* 
     * Define datatype for the data in the file.
     * We will store little endian INT numbers.
     */
    datatype = H5Tcopy(H5T_NATIVE_INT);
    status = H5Tset_order(datatype, H5T_ORDER_LE);

    /*
     * Create a new dataset within the file using defined dataspace and
     * datatype and default dataset creation properties.
     */
    dataset = H5Dcreate(file, DATASETNAME, datatype, dataspace,H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data to the dataset using default transfer properties.
     */
    status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
		      H5P_DEFAULT, data);

    /*
     * Close/release resources.
     */
    H5Sclose(dataspace);
    H5Tclose(datatype);
    H5Dclose(dataset);
    H5Fclose(file);
 
    return 0;
}     