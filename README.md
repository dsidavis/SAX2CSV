# SAX2CSV
Convert XML files that are simple flat hierarchies of attribute values to CSV

```
sax2csv [--trim] [--noout]  inputFile  outputFile  colName colName colName ...
```
outputFile can be "" or - to indicate the results should be written to stdandard output,
where they can be redirected.

--trim replace \r and \n characters with a space

--noout no data will be written to a file or stdout. Instead, just a pass over the data
        is performed to identify any additional attribute names that were not specified in
        calling this function. This can be used to find all the attribute names.

If --noout is specfied, no outputFile should be provided on the command line. 
This is different from "" or '', e.g.,
```
   sax2csv  --noout Badges.xml "" Id UserId Name Date
```
Here the "" implies writing the data to stdout.

So with --noout being used, the command line is 
```
   sax2csv [--trim] --noout  inputFile  colName colName colName ...
```       




We can output all of the attributes found across the XML elements by just providing the name of the
input file, e.g.,
```
sax2csv Badges.xml
```
We can then use these to specify the names of the columns in a second pass
```
sax2csv Badges.xml output `sax2csv Badges.xml`
```
or write the column names to a file and then cat these on the commmand line (in bash)
```
sax2csv Badges.xml  2> COL_NAMES 
sax2csv Badges.xml output `cat COL_NAMES`
```






## postsForTag

Using the Stats site from stackexchange, we can find the posts that have r as a tag with
```
./postsForTag --tag r --noheader  ~/Data/stackexchange/Stats/Posts.xml r.csv Id Tags PostTypeId ParentId AcceptedAnswerId Body
```

+ We specify the tag of interest with --tag.
+ --noheader removes the column names, e.g., for use with a database that doesn't want them
+ Name of the XML Posts file 
+ name of the output TSV (tab-separated values) file
+ the names of the tags we want from each row.  The list above is just for testing.


