#include "mpi.h"
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	
}pixel;

typedef struct {
	char type;
	int width;
	int height;
	int maxValue;
	void* matrix;
}image;

int numtasks;
float IDENTITY[9] = {0, 0, 0, 0, 1, 0, 0, 0, 0};
float SMOOTH[9] = {1.0/9, 1.0/9, 1.0/9, 1.0/9, 1.0/9, 1.0/9, 1.0/9, 1.0/9, 1.0/9 };
float BLUR[9] = {1.0/16, 2.0/16, 1.0/16, 2.0/16, 4.0/16, 2.0/16, 1.0/16, 2.0/16, 1.0/16};
float SHARPEN[9] = {0, -2.0/3, 0, -2.0/3, 11.0/3, -2.0/3, 0, -2.0/3, 0};
float MEAN[9] = {-1, -1, -1, -1, 9, -1, -1, -1, -1};
float EMBOSS[9] = {0, 1, 0, 0, 0, 0, 0, -1, 0}; 

/*
	Read the image
	fileName - image name
	img - the image that will be read
*/
void readInput(const char * fileName, image *img) {
	char filteredImage[10];
	FILE *file;

	file = fopen(fileName,"r");

	//Read type
	fscanf(file, "P%s\n",filteredImage);
	//Set type
	img->type = atoi(filteredImage);

	//Read width
	fscanf(file, "%s ",filteredImage);
	//Set width
	img->width = atoi(filteredImage);

	//Read height
	fscanf(file, "%s ",filteredImage);
	//Set height
	img->height = atoi(filteredImage);

	//Read max value
	fscanf(file, "%s ",filteredImage);
	//Write max value
	img->maxValue = atoi(filteredImage);


	//For color images		
	if(img->type == 6){	
		img->matrix = malloc(img->width * img->height * sizeof(pixel));	
		fread(img->matrix, sizeof(pixel), img->width * img->height, file );
	}
	//For black and white images
	else{
		img->matrix = malloc(img->width * img->height * sizeof(char));	
		fread(img->matrix, sizeof(char), img->width * img->height, file );
	}
	fclose(file);
}

/*
	Write the image
	fileName - image name
	img - the image that wil be wtriten 
*/
void writeData(const char * fileName, image *img) {
	FILE *file;

	//open file
	file = fopen(fileName, "w");  

	//Write image details such as type and height
	fprintf(file, "P%d\n%d %d\n%d\n", img->type, img->width, img->height, img->maxValue);

	//Write matrix
	if(img->type == 6){
		fwrite(img->matrix, sizeof(pixel), img->height * img->width, file );
	}
	else{
		fwrite(img->matrix, sizeof(char), img->height * img->width, file );	
	}

	fclose(file);
}

void aplyFilter_bwImage(int w, int h, int position, unsigned char* input, unsigned char* output, float* FILTER){
	unsigned char* original = input;
	unsigned char* filtered = output;

	int i, j, width, height;
	for(i = 1; i < h - 1; i++){
		for(j = 1; j < w - 1; j++) {
			float p = 0;
				for(height = i - 1; height < (i + 2); height++){
					for(width = j - 1; width < (j + 2); width++) {
						p += original[height * w + width] * FILTER[(height - i + 1) *  3 + (width - j + 1)];						
					}
				}
				if( position == 0){
					if( i == h - 2 && numtasks > 1){
						continue;
					}
					filtered[i * w + j] = p ;
				}
				else{
					filtered[(i - 1) * w + j] = p ;		
				}			

		}
	}

	//up border
	if(position == 0){
		for(j = 0; j < w; j++) {

					filtered[j] = original[j] ;
		}
	}
	
	//down border
	if(position == (numtasks -1)){
		if (position > 0){
			for(j = 0; j < w ; j++) {
				filtered[(h - 2) * w + j] = original[(h - 1) * w + j] ;
			}
		}
		else{
			for(j = 0; j < w ; j++) {
				filtered[(h - 1) * w + j] = original[(h - 1) * w + j] ;
			}
		}
	}

	//left and right border
	for(j = 1; j < h - 1; j++) {
			int x = j;
			if(position > 0){
				x -= 1;
			}
			if( j == h - 2 && position == 0 && numtasks > 1){
				continue;
			}		

			//left border
			filtered[x * w] = original[j * w] ;

			//right border
			filtered[x * w + w - 1] = original[j * w + w - 1] ;
	}
}


void aplyFilter_colorImage(int w, int h, int position, unsigned char* input, unsigned char* output, float* FILTER){
	
	pixel* original = (pixel* )input;
	pixel* filtered = (pixel* )output;

	int i, j, width, height;
	for(i = 1; i < h - 1; i++){
		for(j = 1; j < w - 1; j++) {
			float r = 0, g = 0, b = 0;
				for(height = i - 1; height < (i + 2); height++){
					for(width = j - 1; width < (j + 2); width++) {
						r += original[height * w + width].r * FILTER[(height - i + 1) *  3 + (width - j + 1)];						
						g += original[height * w + width].g * FILTER[(height - i + 1) *  3 + (width - j + 1)]; 
						b += original[height * w + width].b * FILTER[(height - i + 1) *  3 + (width - j + 1)];
					}
				}
				if( position == 0){
					if( i == h - 2 && numtasks > 1){
						continue;
					}
					filtered[i * w + j].r = r ;
					filtered[i * w + j].g = g ;
					filtered[i * w + j].b = b ;
				}
				else{
					filtered[(i - 1) * w + j].r = r ;
					filtered[(i - 1) * w + j].g = g ;
					filtered[(i - 1) * w + j].b = b ;		
				}			

		}
	}

	//up border
	if(position == 0){
		for(j = 0; j < w; j++) {

					filtered[j].r = original[j].r ;
					filtered[j].g = original[j].g ;
					filtered[j].b = original[j].b ;
		}
	}
	
	//down border
	if(position == (numtasks -1)){
		if (position > 0){
			for(j = 0; j < w ; j++) {
				filtered[(h - 2) * w + j].r = original[(h - 1) * w + j].r ;
				filtered[(h - 2) * w + j].g = original[(h - 1) * w + j].g ;
				filtered[(h - 2) * w + j].b = original[(h - 1) * w + j].b ;
			}
		}
		else{
			for(j = 0; j < w ; j++) {
				filtered[(h - 1) * w + j].r = original[(h - 1) * w + j].r ;
				filtered[(h - 1) * w + j].g = original[(h - 1) * w + j].g ;
				filtered[(h - 1) * w + j].b = original[(h - 1) * w + j].b ;
			}
		}
	}

	//left and right border
	for(j = 1; j < h - 1; j++) {
			int x = j;
			if(position > 0){
				x -= 1;
			}
			if( j == h - 2 && position == 0 && numtasks > 1){
				continue;
			}		

			//left border
			filtered[x * w].r = original[j * w].r ;
			filtered[x * w].g = original[j * w].g ;
			filtered[x * w].b = original[j * w].b ;

			//right border
			filtered[x * w + w - 1].r = original[j * w + w - 1].r ;
			filtered[x * w + w - 1].g = original[j * w + w - 1].g ;
			filtered[x * w + w - 1].b = original[j * w + w - 1].b ;
	}
}

int main(int argc, char * argv[]) {

	int rank, tag = 1;
	
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

	unsigned char *recivedImage;
	unsigned char *filteredImage;
	unsigned char *outputMatrix;
	
	int width ;
	int height;
	int lastHeight;
	unsigned char type;

	image input;
	image output;

	int numOfChannels;

	long bytesRecivedImage;
	long bytesFilteredImage;
	long oneLine;
	long bytesLastRecivedImage;
	long bytesFirstFilteredImage;
	long bytesFirstRecivedImage;
	long bytesLastFilteredImage;

	if(rank == 0){

	  	//read the image
		readInput(argv[1], &input);
		
		//store the pixel into a string
		unsigned char* matrix = input.matrix;

		int d = input.height / numtasks;
		int q = input.height % numtasks;
      
		//Set the height for each task
		height = d + 2;			
		lastHeight = d + q + 1; 
		
		//Set the width    
		width = input.width;

		//type of channels
		if(input.type == 5){
			numOfChannels = sizeof(char);
		}
		else{
			numOfChannels = 3 * sizeof(char);
		}	

		//set the type of chars shared for each type of filteredImage	
		oneLine = width  * numOfChannels ;

		bytesRecivedImage       = oneLine * height;
		bytesFilteredImage      = oneLine * (height - 2);
		bytesFirstFilteredImage = oneLine * (height - 2);
		bytesFirstRecivedImage  = oneLine * height;  
		bytesLastRecivedImage   = oneLine * lastHeight;
		bytesLastFilteredImage  = oneLine * (lastHeight - 1);

		if(numtasks == 1){
			height = input.height;
			bytesFirstFilteredImage = height * oneLine;
			bytesFirstRecivedImage  = height * oneLine;
		}

		//Scatter the image
		for (int destination = 1; destination < numtasks; ++destination){
			
			if(destination < numtasks - 1){
				//send type of chars to be recived
				MPI_Ssend (&bytesRecivedImage, 1, MPI_LONG, destination, tag, MPI_COMM_WORLD);
				
				//send type of chars to be stored
				MPI_Ssend (&bytesFilteredImage, 1, MPI_LONG, destination, tag, MPI_COMM_WORLD);			
				
				//send height	
				MPI_Ssend (&height, 1, MPI_INT, destination, tag, MPI_COMM_WORLD);
			}
			else{
				//send type of chars to be recived
				MPI_Ssend (&bytesLastRecivedImage, 1, MPI_LONG, destination, tag, MPI_COMM_WORLD);

				//send type of chars to be stored
				MPI_Ssend (&bytesLastFilteredImage, 1, MPI_LONG, destination, tag, MPI_COMM_WORLD);			
				
				//send height
				MPI_Ssend (&lastHeight, 1, MPI_INT, destination, tag, MPI_COMM_WORLD);
			}
			//send width
			MPI_Ssend (&input.width, 1, MPI_INT, destination, tag, MPI_COMM_WORLD);

			//send type
			MPI_Ssend (&input.type, 1, MPI_CHAR, destination, tag, MPI_COMM_WORLD);

		}

		outputMatrix = malloc(input.height * input.width * numOfChannels);
		recivedImage = malloc(bytesFirstRecivedImage); 	
		filteredImage = malloc(bytesFirstFilteredImage);  


		for(int j = 3; j < argc; j++){

			//Scatter the image
			for (int destination = 1; destination < numtasks; ++destination){
				long from = destination * bytesFilteredImage - oneLine;

				if(destination < numtasks - 1){
					MPI_Ssend (from + matrix, bytesRecivedImage, MPI_CHAR, destination, tag, MPI_COMM_WORLD);
				}
				else{
					MPI_Ssend (from + matrix, bytesLastRecivedImage, MPI_CHAR, destination, tag, MPI_COMM_WORLD);
				}
			}
			memcpy (recivedImage, matrix, bytesFirstRecivedImage);

			//Aply filter
			if(input.type == 5){
				if(strcmp(argv[j],"emboss") == 0){
					aplyFilter_bwImage(width, height, rank, recivedImage, filteredImage, EMBOSS);
				}
				else if(strcmp(argv[j],"mean") == 0){
					aplyFilter_bwImage(width, height, rank, recivedImage, filteredImage, MEAN);
				}
				else if(strcmp(argv[j],"blur") == 0){
					aplyFilter_bwImage(width, height, rank, recivedImage, filteredImage, BLUR);
				}
				else if(strcmp(argv[j],"sharpen") == 0){
					aplyFilter_bwImage(width, height, rank, recivedImage, filteredImage, SHARPEN);
				}
				else if(strcmp(argv[j],"smooth") == 0){
					aplyFilter_bwImage(width, height, rank, recivedImage, filteredImage, SMOOTH);
				}
			}
			else{
				if(strcmp(argv[j],"emboss") == 0){
					aplyFilter_colorImage(width, height, rank, recivedImage, filteredImage, EMBOSS);
				}
				else if(strcmp(argv[j],"mean") == 0){
					aplyFilter_colorImage(width, height, rank, recivedImage, filteredImage, MEAN);
				}
				else if(strcmp(argv[j],"blur") == 0){
					aplyFilter_colorImage(width, height, rank, recivedImage, filteredImage, BLUR);
				}
				else if(strcmp(argv[j],"sharpen") == 0){
					aplyFilter_colorImage(width, height, rank, recivedImage, filteredImage, SHARPEN);
				}
				else if(strcmp(argv[j],"smooth") == 0){
					aplyFilter_colorImage(width, height, rank, recivedImage, filteredImage, SMOOTH);
				}
			}
			
			//Gather the image
			memcpy(outputMatrix, filteredImage, bytesFirstFilteredImage);
			
			for (int i = 1; i < numtasks; ++i)
			{	
				long to = i * bytesFilteredImage;	
				if(i == numtasks - 1){
					MPI_Recv (to + outputMatrix , bytesLastFilteredImage, MPI_CHAR, i, tag, MPI_COMM_WORLD, &status);
				}else{
					MPI_Recv (to + outputMatrix , bytesFilteredImage, MPI_CHAR, i, tag, MPI_COMM_WORLD, &status);
				}
			}

			//Make the new image the input image
			memcpy (matrix, outputMatrix, input.width * input.height * numOfChannels);
		}

		//Write Image
		output.width    = input.width;
		output.height   = input.height;
		output.type     = input.type;
		output.maxValue = input.maxValue;
		output.matrix   = outputMatrix;
		writeData(argv[2], &output);
		
	}

	else if (rank < numtasks){

		//recive type of chars to be recived
		MPI_Recv (&bytesRecivedImage, 1, MPI_LONG, 0, tag, MPI_COMM_WORLD, &status);

		//recive type of chars to be stored
		MPI_Recv (&bytesFilteredImage, 1, MPI_LONG, 0, tag, MPI_COMM_WORLD, &status);

		//recive height
		MPI_Recv (&height, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);

		//recive width
		MPI_Recv (&width, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);

		//recive type
		MPI_Recv (&type, 1, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status);

		recivedImage = (unsigned char*)malloc(bytesRecivedImage);   
		filteredImage  = (unsigned char*)malloc(bytesFilteredImage);   

		for(int j = 3; j < argc; j++){
			//recive image
			MPI_Recv (recivedImage, bytesRecivedImage, MPI_CHAR, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);

			//Aply filter
			if(type == 5){
				if(strcmp(argv[j],"emboss") == 0){
					aplyFilter_bwImage(width, height, rank, recivedImage, filteredImage, EMBOSS);
				}
				else if(strcmp(argv[j],"mean") == 0){
					aplyFilter_bwImage(width, height, rank, recivedImage, filteredImage, MEAN);
				}
				else if(strcmp(argv[j],"blur") == 0){
					aplyFilter_bwImage(width, height, rank, recivedImage, filteredImage, BLUR);
				}
				else if(strcmp(argv[j],"sharpen") == 0){
					aplyFilter_bwImage(width, height, rank, recivedImage, filteredImage, SHARPEN);
				}
				else if(strcmp(argv[j],"smooth") == 0){
					aplyFilter_bwImage(width, height, rank, recivedImage, filteredImage, SMOOTH);
				}
			}
			else{
				if(strcmp(argv[j],"emboss") == 0){
					aplyFilter_colorImage(width, height, rank, recivedImage, filteredImage, EMBOSS);
				}
				else if(strcmp(argv[j],"mean") == 0){
					aplyFilter_colorImage(width, height, rank, recivedImage, filteredImage, MEAN);
				}
				else if(strcmp(argv[j],"blur") == 0){
					aplyFilter_colorImage(width, height, rank, recivedImage, filteredImage, BLUR);
				}
				else if(strcmp(argv[j],"sharpen") == 0){
					aplyFilter_colorImage(width, height, rank, recivedImage, filteredImage, SHARPEN);
				}
				else if(strcmp(argv[j],"smooth") == 0){
					aplyFilter_colorImage(width, height, rank, recivedImage, filteredImage, SMOOTH);
				}
			}
				
			//Sent filtered image
			MPI_Ssend (filteredImage, bytesFilteredImage, MPI_CHAR,0, tag, MPI_COMM_WORLD);
		}
	}

   MPI_Finalize();
}
