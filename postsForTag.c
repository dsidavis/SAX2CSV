#include <libxml/SAX.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>

#include <stdio.h>
#include <string.h>

//#define SHOW(x, ...) fprintf(stderr, x, __VA_ARGS__);
#define SHOW(x, ...) ;

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
    long numEntries;
    long maxEntries;
    char *separator;
    int Trim;

    int repIdTags;
    int noHeader;

    long maxNumPosts;
    long *indicesToCollect;
    char *tagTarget;
} ParserData;

//int Trim = 0;
// long numRecords = -1;
// char *separator = "\t";

int showHelp();

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
showNewCols(ParserData *data, int showBanner)
{
  int i;
  if(data->numNewCols) {
      if(showBanner)
	  fprintf(stderr, "Additional attributes found\n");
      for(i = 0; i < data->numNewCols; i++)
	  fprintf(showBanner ? stderr : stdout, "%s%s", data->newColNames[i], showBanner ? "\n" : "\n");
  }

}

void
writeChars(FILE *out, char *str)
{
    char *p = str;
    fputc('"', out);
    while(*p) {
	
	if( *p == '\r') { // && p[1] && p[1] == '\n')) {
	    //fprintf(stderr, "Skipping control-feed '%s'\n", p+1);
	    p++;
	}
	    
	switch( *p) {
	case '\n':
	    fprintf(out, "\\\\n");
	    break;
	case '\t':
	    fprintf(out, "    ");//XXX tab to 4 spaces! so can use tab separator.
	    break;
	case '"':
	    fprintf(out, "\"");
	    break;
	default: 
	    fputc(*p, out);
	    break;
	}
	    p++;
    }
    fputc('"', out);
}

int
writeValues(ParserData *data)
{
    if(!data->out)
	return(-1);
    int i;

    if(data->repIdTags) {
	
	char *ptr = data->values[1];
	if(!ptr || !ptr[0])
	    return(0);

	int n = 0;
	while(ptr && ptr[0]) {
	    if(ptr[0] == '<') 
   	        fprintf(data->out, "%s%s%s", n++ > 0 ? "\n" : "", data->values[0], data->separator);
	    else if(ptr[0] != '>')
		fputc(ptr[0], data->out);

	    ptr++;
	}
	fputc('\n', data->out);
	return(0);
    }


    for(i = 0; i < data->numColNames; i++) {
      if(data->values[i]) {
	if(data->Trim)
	  trim(data->values[i]);
        writeChars(data->out, data->values[i]);
	// fprintf(data->out, "%s", data->values[i]);	
	data->values[i] = NULL;
      }
	fprintf(data->out, "%s", (i < data->numColNames - 1) ? data->separator : "\n");
    }
    return(0);
}


void 
startDoc(void *ctx)    
{
    ParserData *data = (ParserData*) ctx;
    if(!data->out || data->noHeader)
      return;

    int i;
    // could write the header line in the routine that fopen()s the file
    for(i = 0; i < data->numColNames; i++)
	fprintf(data->out, "%s%s", data->colNames[i], (i < data->numColNames - 1) ? data->separator : "\n");
}

void 
endDoc(void *ctx)
{
    ParserData *data = (ParserData*) ctx;
    if(data->out && data->out != stdout)
      fclose(data->out); // could do this in the same routine that we fopen() the file
                         // for symmetry
}

int
addTagIndices(const xmlChar *attrs, int postType, long index, long accAnswer, ParserData *data)
{

    /* So we have matched the target tag in the Tags element of the attrs. 
       
     */
    addTagIndex(index, data);
    const xmlChar **ptr = attrs;
    if(accAnswer > -1) {
	//fprintf(stderr, "including id = %ld as AcceptedAnwer for %ld\n", accAnswer, index);    
	addTagIndex(accAnswer, data);
    }
#if 0    
    while(ptr && ptr[0]) {

	if(strcmp(ptr[0], "AcceptedAnswerId") == 0 && isTargetIndex(index, data)) {
	    addTagIndex(atol(ptr[1]), data);
	}
	ptr += 2;
    }
#endif
    
    return(0);
}

int
addTagIndex(long index, ParserData *data)
{
    data->indicesToCollect[index - 1] = 1;
    SHOW("Will include id = %ld\n", index);    

    return(0);
}

int
containsTargetTag(const xmlChar *tags, const char *target)
{
    int i, len;
    char *ptr = (char *) tags;
    int nchar = strlen(target);
    while(ptr && *ptr) {
	if(ptr[0] == '<' && (strncmp(ptr + 1, target, nchar) == 0))
	    return(1);
	ptr++;
    }
    return(0);
}

int
isTargetIndex(long idx, ParserData *data)
{
    return(data->indicesToCollect[idx - 1]);
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
    int writeRecord = 0;
    long id = -1, accAnswer = -1;
    int postType = -1;
    
    for(i = 0; i < data->numColNames; i++) {
	data->values[i] = NULL;
    }

    while(ptr && ptr[0]) {
	/* 

         */

	SHOW("id = %ld, attr = %s, postType = %d\n", id, ptr[0], postType);
	if(strcmp(ptr[0], "Id") == 0) {
	    id = atol(ptr[1]);
	    SHOW("Processing id = %ld\n", id);
	} else if(strcmp(ptr[0], "Tags") == 0 && containsTargetTag(ptr[1], data->tagTarget)) {
	    SHOW("found %s in %s\n", data->tagTarget, ptr[1]);
	    addTagIndices(atts, postType, id, accAnswer, data);
	} else if(strcmp(ptr[0], "PostTypeId") == 0)
	    postType = atoi(ptr[1]);
	else if(postType == 1 && strcmp(ptr[0], "AcceptedAnswerId") == 0)  { //&& isTargetIndex(id, data))
	    accAnswer = atol(ptr[1]);
	    SHOW("adding AcceptedAnswerId %ld\n", atol(ptr[1]));
	    if(isTargetIndex(id, data))
		addTagIndex(atol(ptr[1]), data);
	}
	else if(postType == 2 && strcmp(ptr[0], "ParentId") == 0) {
	    long pid = atol(ptr[1]);
	    if(isTargetIndex(pid, data))
		addTagIndex(id, data);
	}
	
	for(i = 0; i < data->numColNames; i++) {
	    if(strcmp(ptr[0], data->colNames[i]) == 0) {
		data->values[i] = ptr[1];
		bad = 0;
		data->colCounts[i]++;
		break;
	    }
	}
	
	ptr += 2;
    }

    writeRecord = data->indicesToCollect[id - 1];
//    if(id == 8713)
//	fprintf(stderr, "about to process 8713 - writeRecord = %d\n", writeRecord);    
    
    // now put the values for this row into the CSV file
    if(writeRecord)
	writeValues(data);

    data->numEntries++;
    if(data->maxEntries > -1 &&  data->numEntries >= data->maxEntries) {
	fflush(data->out);
	fclose(data->out);
	exit(0); // stop more elegantly
    }
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
parse_xml_file(const char *infileName, char *outfileName,  char **colNames, int numColNames, ParserData data) 
{
    xmlParserCtxtPtr ctx; 
    FILE *out;

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
    data.numEntries = 0;

    
    if(numColNames > 0) {
       data.values = (xmlChar **) calloc(numColNames, sizeof(xmlChar *));
       if(!data.values) {
         fprintf(stderr, "cannot allocate memory for column values");
         exit(1);
       }
       data.colCounts = (long*) calloc(numColNames, sizeof(long));
       if(!data.colCounts) {
	   fprintf(stderr, "cannot allocate memory for column counts\n");
	   exit(2);
       }
    }


    data.indicesToCollect = calloc(data.maxNumPosts, sizeof(long));
    if(!data.indicesToCollect) {
	fprintf(stderr, "cannot allocate memory for indices to collect\n");
	exit(3);
    }

    ctx = xmlCreateFileParserCtxt(infileName);
    ctx->userData = &data;
    ctx->sax = &methods;
    
    xmlParseDocument(ctx);

    showColCounts(&data);
    if(data.numNewCols) 
	showNewCols(&data, numColNames > 0);
    

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
  ParserData parserData;
  parserData.separator = strdup("\t");
  parserData.Trim = 0;
  parserData.repIdTags = 0;
  parserData.maxEntries = -1;
  parserData.noHeader = 0;
  parserData.maxNumPosts = 54e6;

  if(argc == 1) {
      showHelp();
      exit(1);
  }
  
  for(i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
       offset ++;
       if(strcmp(argv[i], "--help") == 0) {
	   showHelp();
	   exit(1);
       } else if(strcmp(argv[i], "--trim") == 0) {
   	  fprintf(stderr, "enabling trimming\n");
	  parserData.Trim = 1;
       } else if(strcmp(argv[i], "--noout") == 0) {
	  noout = 1;
       } else if(strcmp(argv[i], "--tag") == 0) {
	   parserData.tagTarget = malloc(sizeof(char) * (strlen(argv[i+1]) + 2));
	   memcpy(parserData.tagTarget, argv[i+1], sizeof(char) * strlen(argv[i+1]));
	   parserData.tagTarget[ strlen(argv[i+1]) ] = '>';
	   parserData.tagTarget[ strlen(argv[i+1]) + 1] = '\0';
	   // fprintf(stderr, "looking for '%s' in Tags attribute\n", parserData.tagTarget);
 	   i++;
	  offset++;
       } else if(strcmp(argv[i], "--noheader") == 0) {
	  parserData.noHeader = 1;
       } else if(strcmp(argv[i], "--num") == 0 && argc > i+1) {
	   parserData.maxEntries = atol(argv[i+1]);
	   i++;
	   offset ++;
       } else if(strcmp(argv[i], "--sep") == 0 && argc > i+1) {
	   parserData.separator = strdup(argv[i+1]);
	   i++;
	   offset ++;
       } else if(strcmp(argv[i], "--numPosts") == 0 && argc > i+1) {
	   parserData.maxNumPosts = atol(argv[i+1]);
	   i++;
	   offset ++;
       }

    } else 
      break;
  }

  return( parse_xml_file(argv[offset+1], noout ? NULL : argv[offset + 2], argv + (offset + 3 - noout), argc - (3 + offset - noout), parserData ));
    
}

int
showHelp()
{
    fprintf(stderr, "postsForTag [--trim] [--noout] [--tag tag] inputFile  outputFile  colName colName colName ...\n");
    fprintf(stderr, "\n--trim  remove white space before and after (attribute) values");
    fprintf(stderr, "\n--noout  no output is written to a file or stdout, just a pass of the data to identify attribute names");    ;
    fprintf(stderr, "\ninputFile  path to the XML file to be processed");
    fprintf(stderr, "\noutputFile  path for writing the content as CSV. Can be '' for stdout");
    fprintf(stderr, "\ncolName  the name(s) of the attributes to extract from each record and write to the CSV");
    fprintf(stderr, "\ntag the name(s) of the tag to find in the Tags attribute to identify records of interest");
    fprintf(stderr, "\n");
}
