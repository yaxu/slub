/* File: sendOSC.i */
%module sendOSC
%{
/* bla */
#include "OSC-client.h"
#include "htmsocket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

// This tells SWIG to treat char ** as an array

%typemap(perl5,in) char ** {
	AV *tempav;
	I32 len;
	int i;
	SV  **tv;

	if (!SvROK($source))
	    croak("$source is not an array.");
        if (SvTYPE(SvRV($source)) != SVt_PVAV)
	    croak("$source is not an array.");
        tempav = (AV*)SvRV($source);
	len = av_len(tempav);
	$target = (char **) malloc((len+2)*sizeof(char *));
	for (i = 0; i <= len; i++) {
	    tv = av_fetch(tempav, i, 0);	
	    $target[i] = (char *) SvPV(*tv,PL_na);
        }
	$target[i] = 0;
};

// This cleans up our char ** array after the function call

%typemap(perl5,freearg) char ** {
	free($source);
}

// Creates a new Perl array and places a char ** into it

%typemap(perl5,out) char ** {
	AV *myav;
	SV **svs;
	int i = 0,len = 0;
	/* Figure out how many elements we have */
	while ($source[len])
	   len++;
	svs = (SV **) malloc(len*sizeof(SV *));
	for (i = 0; i < len ; i++) {
	    svs[i] = sv_newmortal();
	    sv_setpv((SV*)svs[i],$source[i]);
	};
	myav =	av_make(len,svs);
	for (i = 0; i < len; i++) {
	  /*	  SvREFCNT_dec(svs[i]); */
	}
	free(svs);
        $target = newRV((SV*)myav);
        sv_2mortal($target);
	argvi++;                      /* This is critical! */
}

		
%inline %{

int print_args(char **argv) {
    int i = 0;
    while (argv[i]) {
         printf("argv[%d] = %s\n", i,argv[i]);
         i++;
    }
    return i;
}

// Returns a char ** list 

char **get_args() {
    static char *values[] = { "Dave", "Mike", "Susan", "John", "Michelle", 0};
    return &values[0];
}


%}

extern void HartCoreMode(int argc, char *argv[]);
