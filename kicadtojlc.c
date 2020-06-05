/*
 ============================================================================
 Name        : kicadtojlc.c
 Author      : Murray Horn
 Version     : 0.0
 Copyright   : GPLV2

     This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.




 Description : Conversion of Kicad .pos files to JLCPCB compatible .cpl and .bom files.
  	  	  	   A footprint renaming, and rotation configuration file is supported.
  	  	  	   This allows standard KICAD footprints to translate to JLCpcb's placement system.
  	  	  	   This file is plain text and can be user modified to accommodate all types of components.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// we just assume a sane value.
#define MAX_LINE_LENGTH 300


int Input_Fail;


enum {STATE_INIT,STATE_GET_COMPONENT,STATE_GET_POSITION,STATE_FIN};
int State = STATE_INIT;


void packages_swap(char *field,char *from,char *to){
	if (0 == strcmp(field,from))
		strcpy(field,to);
}



//----------------------------------------------------------------------------------------------------------------

void convert_line(FILE *f_conv,char *part,char *value,char *package,float *rotate_pnt){
	char 	new_package[MAX_LINE_LENGTH];
	char 	old_package[MAX_LINE_LENGTH];
	float	rotate_offset,old_rotate;
	char 	text_line[MAX_LINE_LENGTH];
	int 	linecount;


   if (f_conv != NULL)
   {
	   linecount = 0;
	   fseek(f_conv,0,0);
	   while (0 == feof(f_conv))
	   {
		   if( fgets (text_line, MAX_LINE_LENGTH, f_conv)!=NULL ) {
			   linecount++;
			   if (linecount > 1){
				   if (3 == sscanf(text_line, "%s %s %f", old_package, new_package,&rotate_offset))
				   {
					   if (0 == strcmp(old_package,package))   {
						   strcpy(package,new_package);
						   old_rotate = *rotate_pnt;
						   *rotate_pnt = old_rotate + rotate_offset;
						   printf("%s changed from %s angle %f to %s angle %f\n",part,old_package, old_rotate, new_package,*rotate_pnt);
					   }
				   }
			   }
		   }
	   }
   }
}
//----------------------------------------------------------------------------------------------------------------


void process_line(FILE *f_cpl,FILE *f_bom,FILE *f_conv,char* line_pnt,int First_Bom_Line){
	char 	part[MAX_LINE_LENGTH];
	char 	value[MAX_LINE_LENGTH];
	char 	package[MAX_LINE_LENGTH];
	char 	layer[MAX_LINE_LENGTH];


	char 	part_memory[MAX_LINE_LENGTH];
	char 	value_memory[MAX_LINE_LENGTH];
	char 	package_memory[MAX_LINE_LENGTH];

	int 	match;
	float	pos_x,pos_y,rotate;

//	fprintf(stdout, "LINE=%s\n", line_pnt);

	if (State == STATE_INIT)
	{
		State = STATE_GET_COMPONENT;
		fputs("Comment,Designator,Footprint,LCSC Part #\n",f_bom);
		fputs("Designator,Mid X,Mid Y,Layer,Rotation\n",f_cpl);

	}

	switch (State) {

		case STATE_GET_COMPONENT:

// an initial scan to check for the end of file!
			sscanf(line_pnt, "%s %s %s %f %f %f %s", part, value, package, &pos_x, &pos_y, &rotate, layer);

			   if ((0 == (strcmp(part,"##"))) && (0 == (strcmp(value,"End"))))
			   {
					State = STATE_FIN;
			   }
			   else {


					//----------------------------------
					// loading defaults!
					strcpy(part,"ERROR!");
					strcpy(value,"ERROR!");
					strcpy(package,"ERROR!");
					pos_x = 0xffffffff;
					pos_y = 0xffffffff;
					rotate = 0xffffffff;
					strcpy(layer,"ERROR!");
					//----------------------------------

// the real scan with parameter check
				   if (7 != sscanf(line_pnt, "%s %s %s %f %f %f %s", part, value, package, &pos_x, &pos_y, &rotate, layer))
				   {
					   printf("\nERROR the following line did not scan correctly :\n");
					   printf("   %s\n",line_pnt);
					   printf("   translated as\n");
					   printf("   part = %s\n",part);
					   printf("   value = %s\n",value);
					   printf("   package = %s\n",package);
					   printf("   X position = %f\n",pos_x);
					   printf("   Y position = %f\n",pos_y);
					   printf("   rotate = %f\n",rotate);
					   printf("   layer = %s\n",layer);

					   printf("\n perhaps there was a space within one of the fields.\n\n");
					   Input_Fail = 1;
				   }




				   convert_line(f_conv,part,value,package,&rotate);

//-------------------------------
// save the component location
				   fprintf(f_cpl,"%s,%fmm,%fmm,%s,%f\n",part,pos_x,pos_y,layer,rotate);

//-------------------------------
// save the component value, we check for duplicates and ignore them
// when panelizing multiple copies of components are recorded, for example C1 100nF
// if duplicates are passed to JCL, their system complains

				   match = 1;

				   if (strcmp(part_memory,part))		match = 0;
				   if (strcmp(value_memory,value))		match = 0;
				   if (strcmp(package_memory,package))	match = 0;

				   strcpy(part_memory,part);
				   strcpy(value_memory,value);
				   strcpy(package_memory,package);

				   if ((match == 0) || (First_Bom_Line))
					   fprintf(f_bom,"%s,%s,%s,\n",value,part,package);
//-------------------------------


			   }

			break;
		default:
			break;
	}
}



int main(int argc, char* argv[])
{

// we just assume a sane value.
#define INPUT_FILE_LENGTH	300

char base_file_name[INPUT_FILE_LENGTH];
char input_filename[INPUT_FILE_LENGTH+8];
char bom_filename[INPUT_FILE_LENGTH+8];
char cpl_filename[INPUT_FILE_LENGTH+8];
int i;
char *pnt;
char 	line[MAX_LINE_LENGTH];

FILE *f_input;
FILE *f_cpl;
FILE *f_bom;
FILE *f_conv;
FILE *f_convert_create;

int	First_BOM;
int leng;
int	line_cnt = 0;


	Input_Fail = 0;

	if (argc == 2){
        strcpy(base_file_name,argv[1]);
	} else {
        printf("enter kicad pos filename without .pos extension\n");
        printf("-help for help\n");

         pnt = fgets(base_file_name, INPUT_FILE_LENGTH, stdin);
         if (pnt == NULL)
         {
        	 printf("Input Error\n");
        	 printf("Exit\n");
        	 return(EXIT_FAILURE);
         }
    	 i = strlen(base_file_name);
    	 if (i) base_file_name[i-1] = 0;
    };




    if ((0 == strcmp("-help",base_file_name))
     || (0 == strcmp("-Help",base_file_name))
	 || (0 == strcmp("-HELP",base_file_name))
	){
    	printf("This application provides a conversion for KICADs .pos files to JLCpcbs .bom and .cpl files.\n");
    	printf("\n");
    	printf("A conversion file called <part_conversion.txt> allows for the swapping of component");
    	printf("package names and offset rotations allowing simpler adoption of KICADs files into JLCpcb.\n");
    	printf("Here is a sample of a <part_conversion.txt> file.\n");
    	printf("\n");
    	printf("package names and offset rotations allowing simpler adoption of KICADs files into JLCpcb.\n");
    	printf("\n");
    	printf("\n use the -convert parameter to output a sample part_conversion.txt file.\n");
    	printf("\n");
    	printf("\n");
    	printf("In Kicad, the .pos file can be produced by clicking.\n");
    	printf(" File ... Fabrication Options ... Footprint Position\n");
    	printf("\n");
    	return(EXIT_FAILURE);
    }


    if (0 == strcmp("-convert",base_file_name))
	{
    	printf("Conversion file creation.\n");
    	f_convert_create = fopen("part_conversion.txt" , "w");
    	if(f_convert_create == NULL)
    	{
    		printf("Error creating part_conversion.txt file\n");
        	return(EXIT_FAILURE);
    	}

    	fprintf(f_convert_create,"%s","from		to		rotate_offset	this top line is ignored by the processors\n");
    	fprintf(f_convert_create,"%s","R_0603		0603_R		0\n");
    	fprintf(f_convert_create,"%s","R_0805		0805_R		0\n");
    	fprintf(f_convert_create,"%s","R_1206		1206_R		0\n");
    	fprintf(f_convert_create,"%s","C_0603		0603_C		0\n");
    	fprintf(f_convert_create,"%s","C_0805		0805_C		0\n");
    	fprintf(f_convert_create,"%s","C_1206		1206_C		0\n");

    	fclose(f_convert_create);

    	printf("Done.\n");
    	return(EXIT_FAILURE);
    }



// ----------------------------------------
// removal of the .pos extension if it is included
	leng = strlen(base_file_name);
	if (leng >= 4)
	{
		if (	(base_file_name[leng-4] == '.')
				&&	(base_file_name[leng-3] == 'p')
				&&	(base_file_name[leng-2] == 'o')
				&&	(base_file_name[leng-1] == 's')
			)
			base_file_name[leng-4] = 0;
	}
// ----------------------------------------


	strcpy(input_filename,base_file_name);		strcat(input_filename,".pos");

   strcpy(input_filename,base_file_name);		strcat(input_filename,".pos");
   strcpy(bom_filename,base_file_name);   		strcat(bom_filename,"_bom.csv");
   strcpy(cpl_filename,base_file_name);   		strcat(cpl_filename,"_cpl.csv");

   f_input = fopen(input_filename, "r");
   if(f_input == NULL){
	   printf("Error opening input file %s\n",input_filename);
	   printf("Exit\n");
	   return(EXIT_FAILURE);
   }

   f_cpl = fopen(cpl_filename , "w");
   f_bom = fopen(bom_filename , "w");

//---------------
	if(f_cpl == NULL)	printf("Error creating cpl file %s\n",cpl_filename);
	if(f_bom == NULL)	printf("Error creating bom file %s\n",bom_filename);
//---------------

   if (f_input != NULL)
       printf("source file %s found\n",input_filename);
//---------------
   f_conv = fopen("part_conversion.txt" , "r");
   if (f_conv == NULL)
	   printf("unable to find part_conversion.txt\n");
   else
	   printf("part_conversion.txt found\n");
//---------------

   if((f_input == NULL) || (f_bom == NULL) || (f_cpl == NULL))
   {
	   if(NULL != f_input)	fclose(f_input);
	   if(NULL != f_cpl)	fclose(f_cpl);
	   if(NULL != f_bom)	fclose(f_bom);
	   if(NULL != f_conv)	fclose(f_conv);
	   printf("Exit\n");
	   return(EXIT_FAILURE);
   }

   First_BOM = 1;
   line_cnt = 0;
   while (0 == feof(f_input))
   {
	   line_cnt++;
	   if( fgets (line, MAX_LINE_LENGTH, f_input)!=NULL ) {
		   if (line_cnt >= 6)
		   {
			   process_line(f_cpl,f_bom,f_conv,line,First_BOM);
			   First_BOM = 0;
		   }
	   }
   }

   if(NULL != f_input)	fclose(f_input);
   if(NULL != f_cpl)	fclose(f_cpl);
   if(NULL != f_bom)	fclose(f_bom);
   if(NULL != f_conv)	fclose(f_conv);

//---------------
   if (Input_Fail)
   {
	   f_cpl = fopen(cpl_filename , "w");
	   if(NULL != f_cpl)	fclose(f_cpl);

	   f_bom = fopen(bom_filename , "w");
	   if(NULL != f_bom)	fclose(f_bom);

	   printf("Error in input file, perhaps spaces within fields");
   } else
	   printf("all done\n");

   return(EXIT_SUCCESS);
}


