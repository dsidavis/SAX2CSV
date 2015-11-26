#include <libxml/SAX.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>

#include <stdio.h>
#include <string.h>

typedef struct {
    const xmlChar **colNames;
    int numColNames;
    const xmlChar **values;
    int ctr;
    FILE * out;
    int go;
    int extraCols;
    long *colCounts
} ParserData;

int Trim = 0;

int
trim(xmlChar *str)
{
  int changed = 0, i;
  xmlChar *p = str;

  for(p = str ; p[0]; p++) {
    if(p[0] == '\r' || p[0] == '\n') {
       p[0] = ' ';
       changed++;
    }
  }
  return(changed);
}

int
writeValues(ParserData *data)
{
    int i;
    for(i = 0; i < data->numColNames; i++) {
      if(data->values[i]) {
	  trim(data->values[i]);
	  fprintf(data->out, "%s", data->values[i]);	
      }
	fprintf(data->out, "%s", (i < data->numColNames - 1) ? "," : "\n");
    }
    return(0);
}


void 
startDoc(void *ctx)    
{
    ParserData *data = (ParserData*) ctx;
    int i;
    for(i = 0; i < data->numColNames; i++)
	fprintf(data->out, "%s%s", data->colNames[i], (i < data->numColNames - 1) ? "," : "\n");
}

void 
endDoc(void *ctx)
{
    ParserData *data = (ParserData*) ctx;
    fclose(data->out);
}

void 
startElement(void *ctx, const xmlChar *name, const xmlChar **atts)    
{
    ParserData *data = (ParserData*) ctx;
    if(data->go == 0) {
	data->go = 1;
	return;
    }

    const xmlChar **ptr = atts;
    int i, bad = 0;
    while(ptr && ptr[0]) {
        bad = 1;
	for(i = 0; i < data->numColNames; i++) {
	    if(strcmp(ptr[0], data->colNames[i]) == 0) {
		data->values[i] = ptr[1];
		bad = 0;
		data->colCounts[i]++;
		break;
	    }
	}
	if(bad) {
	    fprintf(stderr, "new column name %s\n", ptr[0]);
	    data->extraCols = 1;
	}
	ptr += 2;
    }
    writeValues(data);
}



void 
endElement(void *ctx, const xmlChar *name)
{
    // output the values
    ParserData *data = (ParserData*) ctx;
    //    writeValues(data);
}

void chars(void *ctx, const xmlChar *ch, int len)
{
  //    fprintf(stderr, "chars\n");
}



xmlSAXHandler methods  = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, //setDocumentLocator
    startDoc,
    endDoc,
    startElement,
    endElement,
    NULL,
    chars,
    chars, // ignorableWhitespace
    NULL,
    NULL, // comment
    NULL, //warning,
    NULL, // error,
    NULL,
    NULL,
    NULL, // cdata
    NULL,
    1
#if 0
    ,NULL,
    NULL
    NULL,
    NULL
#endif
};


void
showColCounts(ParserData *data)
{
  int i;
  for(i = 0; i < data->numColNames; i++)
    fprintf(stderr, "%s %ld\n", data->colNames[i], data->colCounts[i]);
}


int
parse_xml_file(const char *infileName, char *outfileName,  char **colNames, int numColNames) {

    xmlParserCtxtPtr ctx; 
    FILE *out;
    ParserData data;

    if( ! ( out = fopen(outfileName, "w"))) {
	fprintf(stderr, "cannot open %s\n", outfileName);
	exit(1);
    }
    data.out = out;
    data.ctr = 0;
    data.go = 0;
    data.colNames = colNames;
    data.numColNames = numColNames;
    data.extraCols = 0;
    data.values = (xmlChar **) calloc(numColNames, sizeof(xmlChar *));
    data.colCounts = (long*) calloc(numColNames, sizeof(long));

    ctx = xmlCreateFileParserCtxt(infileName);
    ctx->userData = &data;
    ctx->sax = &methods;
    
    xmlParseDocument(ctx);

    showColCounts(&data);

    ctx->sax = NULL;
    xmlFreeParserCtxt(ctx);

    return(data.extraCols);
}

int 
main(int argc, char **argv)
{
  int i;
  int offset = 0;
  for(i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      offset ++;
      if(strcmp(argv[i], "--trim") == 0) 
	fprintf(stderr, "enabling trimming\n");
	Trim = 1;
    } else 
      break;
  }

  return( parse_xml_file(argv[offset+1], argv[offset + 2], argv + (offset + 3), argc - (3 + offset) ));
    
}
