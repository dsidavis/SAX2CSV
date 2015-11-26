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
