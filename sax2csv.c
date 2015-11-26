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
    xmlChar **newColNames;
    int numNewCols;
    long *colCounts;
} ParserData;

int Trim = 0;

void 
addExtraCol(ParserData *d, const xmlChar *name)
{
  int i;
  for(i = 0; i < d->numNewCols; i++) {
    if(xmlStrcmp(name, d->newColNames[i]) == 0) {
      return;
    }
  }

  // So didn't find it.
  d->numNewCols ++;
  d->newColNames = (xmlChar **) realloc(d->newColNames, d->numNewCols * sizeof(xmlChar *));
  if(!d->newColNames) {
    fprintf(stderr, "cannot allocate space for extra column names\n");
    exit(3);
  }
  d->newColNames[d->numNewCols - 1] = xmlStrdup(name);
  d->extraCols = 1;
}

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

void
showNewCols(ParserData *data)
{
  int i;
  if(data->numNewCols) {
    fprintf(stderr, "Additional attributes found\n");
    for(i = 0; i < data->numNewCols; i++)
      fprintf(stderr, "%s\n", data->newColNames[i]);
  }

}
int
writeValues(ParserData *data)
{
    if(!data->out)
      return;
    int i;
    for(i = 0; i < data->numColNames; i++) {
      if(data->values[i]) {
	if(Trim)
	  trim(data->values[i]);
	fprintf(data->out, "%s", data->values[i]);	
	data->values[i] = NULL;
      }
	fprintf(data->out, "%s", (i < data->numColNames - 1) ? "," : "\n");
    }
    return(0);
}


void 
startDoc(void *ctx)    
{
    ParserData *data = (ParserData*) ctx;
    if(!data->out)
      return;

    int i;
    // could write the header line in the routine that fopen()s the file
    for(i = 0; i < data->numColNames; i++)
	fprintf(data->out, "%s%s", data->colNames[i], (i < data->numColNames - 1) ? "," : "\n");
}

void 
endDoc(void *ctx)
{
    ParserData *data = (ParserData*) ctx;
    if(data->out && data->out != stdout)
      fclose(data->out); // could do this in the same routine that we fopen() the file
                         // for symmetry
}

void 
startElement(void *ctx, const xmlChar *name, const xmlChar **atts)    
{
    ParserData *data = (ParserData*) ctx;
    if(data->go == 0) {
      // the first element we see is the root and so we acknowledge we
      // have seen that and then all subsequent starts of elements
      // are the rows we want.
	data->go = 1;
	return;
    }

    // loop over the attributes (as pairs of name - values)
    // and put the values into the data->values slots in the order
    // of the columns of the CSV file, i.e. colNames.
    // Give a warning if we find an attribute not in colNames.
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
	if(bad) 
	  addExtraCol(data, ptr[0]);
	
	ptr += 2;
    }
    // now put the values for this row into the CSV file
    writeValues(data);
}



void 
endElement(void *ctx, const xmlChar *name)
{
  // Nothing to do here. All done in startElement().
}

void chars(void *ctx, const xmlChar *ch, int len)
{
  // This is called for the text between the <row> elements
  // and we don't care about that. It is just whitespace.
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


// Used at the end to show the number of each of the attributes.
void
showColCounts(ParserData *data)
{
  int i;
  for(i = 0; i < data->numColNames; i++)
    fprintf(stderr, "%s %ld\n", data->colNames[i], data->colCounts[i]);
}


int
parse_xml_file(const char *infileName, char *outfileName,  char **colNames, int numColNames) 
{
    xmlParserCtxtPtr ctx; 
    FILE *out;
    ParserData data;

    if(outfileName == NULL)
      data.out = NULL;
    else if(!outfileName[0] || strcmp(outfileName, "-") == 0)
      data.out = stdout;
    else {
      if( ! ( out = fopen(outfileName, "w"))) {
	fprintf(stderr, "cannot open %s\n", outfileName);
	exit(1);
      }
      data.out = out;
    }

    data.ctr = 0;
    data.go = 0;
    data.colNames = colNames;
    data.numColNames = numColNames;
    data.extraCols = 0;
    data.numNewCols = 0;
    data.newColNames = NULL;

    data.values = (xmlChar **) calloc(numColNames, sizeof(xmlChar *));
    if(!data.values) {
      fprintf(stderr, "cannot allocate memory for column values");
      exit(1);
    }
    data.colCounts = (long*) calloc(numColNames, sizeof(long));
    if(!data.colCounts) {
      fprintf(stderr, "cannot allocate memory for column counts");
      exit(2);
    }

    ctx = xmlCreateFileParserCtxt(infileName);
    ctx->userData = &data;
    ctx->sax = &methods;
    
    xmlParseDocument(ctx);

    showColCounts(&data);
    if(data.numNewCols) 
       showNewCols(&data);


    ctx->sax = NULL;
    xmlFreeParserCtxt(ctx);

    return(data.extraCols);
}

int 
main(int argc, char **argv)
{
  int i;
  int offset = 0;
  int noout = 0;
  for(i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      offset ++;
      if(strcmp(argv[i], "--trim") == 0) {
	fprintf(stderr, "enabling trimming\n");
	Trim = 1;
      } else if(strcmp(argv[i], "--noout") == 0) {
	noout = 1;
      }
    } else 
      break;
  }

  return( parse_xml_file(argv[offset+1], noout ? NULL : argv[offset + 2], argv + (offset + 3 - noout), argc - (3 + offset - noout) ));
    
}
