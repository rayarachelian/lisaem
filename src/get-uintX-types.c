/**************************************************************************************\
*                                                                                      *
*                       libGenerator - integer size detector                           *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2020 Ray A. Arachelian                          *
*                                All Rights Reserved                                   *
*                                                                                      *
\**************************************************************************************/

//    Because there's nothing quite like the thrill of reinventing the wheel again!   //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

int main(int argc, char *argv[])
{
FILE *out;
int sz[5];
int i;
int h[9];

h[0]=0;
h[1]=0;
h[2]=0;
h[3]=0;
h[4]=0;
h[5]=0;
h[6]=0;
h[7]=0;
h[8]=0;

char *szs[5]={"char", "short", "int", "long long", "long"};

char buffer[256];
FILE *uname;

uname=popen("uname -msr","r");
if  (uname) {
    memset(buffer,0,255);
    (void)fread(buffer,254,1,uname);
    fclose(uname);
}

out=fopen("machine.h","wt");
if (!out) {perror("Could not create machine.h because "); return -1; }

sz[0]=sizeof(char);
sz[1]=sizeof(short);
sz[2]=sizeof(int);
sz[3]=sizeof(long long);
sz[4]=sizeof(long);


fprintf(out,
"/**************************************************************************************\\\n"
"*                                                                                      *\n"
"*                machine.h -  detected integer types for this host                     *\n"
"*                              http://lisaem.sunder.net                                *\n"
"*                                                                                      *\n"
"*                   Copyright (C) 1998, 2020 Ray A. Arachelian                         *\n"
"*                                All Rights Reserved                                   *\n"
"*                                                                                      *\n"
"\\**************************************************************************************/\n\n"
"#include <stdio.h>\n"
"#include <stdlib.h>\n"
"#include <sys/types.h>\n\n\n"
"#ifndef IN_MACHINE_H\n"
"#define IN_MACHINE_H 1\n\n\n"
);

fprintf(out,"/*---------------------------------------------\n");
fprintf(out,"  uname -msr: %s\n",buffer);
fprintf(out," *---------------------------------------------*/\n");
fprintf(out,"/* sizeof(char     )=%2d */\n",(int)(sizeof(char     )));
fprintf(out,"/* sizeof(int      )=%2d */\n",(int)(sizeof(int      )));
fprintf(out,"/* sizeof(short    )=%2d */\n",(int)(sizeof(short    )));
fprintf(out,"/* sizeof(long     )=%2d */\n",(int)(sizeof(long     )));
fprintf(out,"/* sizeof(long long)=%2d */\n",(int)(sizeof(long long)));
fprintf(out,"/* sizeof(float    )=%2d */\n",(int)(sizeof(float    )));
fprintf(out,"/* sizeof(double   )=%2d */\n",(int)(sizeof(double   )));
fprintf(out,"/*---------------------------------------------*/\n\n");


for (i=0; i<5; i++) {
      if (! h[sz[i]] ) {
            h[sz[i]]=1;

            fprintf(out,"typedef          %-9s %s  int%d;\n",   szs[i], (sz[i]<2 ? " ":""), sz[i]*8 ); 
            fprintf(out,"typedef          %-9s %s sint%d;\n",   szs[i], (sz[i]<2 ? " ":""), sz[i]*8 ); 
            fprintf(out,"typedef unsigned %-9s %s uint%d;\n",   szs[i], (sz[i]<2 ? " ":""), sz[i]*8 ); 
            if (errno) {perror("couldn't write to machine.h because "); fclose(out); return -1;}
      }
}
fprintf(out,"#endif\n");

fclose(out);

if (h[1] && h[2] && h[4] && h[8]) return 0;
return -1;
}
