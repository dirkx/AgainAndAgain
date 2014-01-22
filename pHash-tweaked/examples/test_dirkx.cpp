/*

    pHash, the open source perceptual hash library
    Copyright (C) 2009 Aetilius, Inc.
    All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Evan Klinger - eklinger@phash.org
    David Starkweather - dstarkweather@phash.org

*/

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <algorithm>
#include <assert.h>

#include "pHash.h"

using namespace std;

#define TRUE 1
#define FALSE 0

//data structure for a hash and id
struct ph_imagepoint{
    ulong64 hash;
    char *id;
};

//aux function to create imagepoint data struct
struct ph_imagepoint* ph_malloc_imagepoint(){

    return (struct ph_imagepoint*)malloc(sizeof(struct ph_imagepoint));

}

/** TEST for image DCT hash function 
 *  The program reads all images from the two directories given on the command line.
 *  The program expects the same number of image files in each directory. Each image 
 *  should be perceptually similar to a corresponding file in the other directory and
 *  have the same exact name.  For example, one directory could contain the originals,
 *  and the other directory blurred counterparts.  The program calculates the hashes.
 *  First, the hamming distances are calculated between all similar image hashes (-i.e. 
 *  the intra compares), and then hamming distances for different images hashes (-i.e. 
 *  the inter compares).
**/
int main(int argc, char **argv){

    const char *msg = ph_about();
    printf(" %s\n", msg);

    if (argc != 2) {
	printf("no input args\n");
	printf("expected: \"test_imagephash <dirname>\".\n");
	exit(1);
    }

    const char *dir_name = argv[1];
    struct dirent *dir_entry;
    vector<ulong64> lst; //for hashes in first directory
    ph_imagepoint *dp = NULL;

    DIR *dir = opendir(dir_name);
    if (!dir){
	printf("unable to open directory\n");
	exit(1);
    }
    errno = 0;
    int i = 0;
	FILE *f = NULL;
    char path[100];
    path[0] = '\0';
    while ((dir_entry = readdir(dir)) != 0)
    {
 	if (strstr(dir_entry->d_name,"hash"))
		continue;

	if (strcmp(dir_entry->d_name,".")==0 || strcmp(dir_entry->d_name,"..")==0) 
		continue;

	
	strcat(path, dir_name);
	strcat(path, "/");
	strcat(path, dir_entry->d_name);

	// check if we already have a hash - and use that if at all possible.
	//
	char tmp[PATH_MAX]; struct stat ign;
	snprintf(tmp,PATH_MAX-1,"%s.hash",path);

    	ulong64 tmphash = 0;
	int max = 0;

	if ((stat(tmp,&ign) != 0) || (!(f=fopen(tmp,"r"))) || fscanf(f,"%llu",&tmphash) != 1 || fclose(f) != 0) 
	{
	   fprintf(stderr,"E: %s %s  %p/%llu %s\n",path,tmp,f,tmphash,strerror(errno));
	    if (ph_dct_imagehash(path, tmphash) < 0) {
		fprintf(stderr,"Error on %s\n",path);
		continue;
	    };

            if (1) {
		f=fopen(tmp,"w"); assert(f);
		fprintf(f,"%llu\n", tmphash);
		fclose(f);
	   } 
	};

	lst.push_back(tmphash);

	if (i++ % 100 == 0) {
	    fprintf(stderr,".");
	    fflush(stderr);
	    if (i % 10000 == 0) 
		fprintf(stderr,"%d",i);
	}
	errno = 0;
        path[0]='\0';
    }

    if (errno){
	printf("error reading directory\n");
	exit(1);
    }


    int nbfiles = lst.size();

    int distance = -1;

    int max = 32;

    f = fopen("tmp.pgm","w"); assert(f);
    int len = fprintf(f,"P5\n%lu %lu\n%d\n",nbfiles,nbfiles,max);

    fprintf(stderr,"\nWriting file %d bytesish\n\n",nbfiles*nbfiles+len);

    for (i=0;i<nbfiles;i++){
	unsigned char buff[nbfiles];
	for (int j=0; j<i; j++)
		buff[j]=max - 1;

	for (int j=i+1;j<nbfiles;j++){
	    distance = ph_hamming_distance(lst[i],lst[j]);
	    if (distance >=max) distance = max -1;
	    buff[j]= distance;
	}
	fwrite(buff,1,nbfiles,f);
    }
    return 0;
}
