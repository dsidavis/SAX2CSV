library(XML)
doc = xmlParse("Posts.xml")

rows = getNodeSet(doc, "//row")
id = sapply(rows, xmlGetAttr, "Id")
parentId = sapply(rows, xmlGetAttr, "ParentId", NA)
names(parentId) = id
accId = sapply(rows, xmlGetAttr, "AcceptedAnswerId", NA)
names(accId) = id

otags = sapply(rows, xmlGetAttr, "Tags")
names(otags) = id
tags = gsub("<", "", gsub(">", ";", otags))
names(tags) = id

tgs = strsplit(tags, ";")
sort(table(unlist(tgs)))


i = grepl("^r;|;r;|r$", tags)
i = grepl("^normality;|;normality;|normality$", tags)

type = sapply(rows, xmlGetAttr, "PostTypeId")
w = type == "2"


j = parentId %in% id[i]
id[j]

# Get the id of the posts which
#  contain the target tag in Tags
#  is an answer for a post with the target tag
#  has a parentId of a post containing the target tag

k = accId[i]
k = k[!is.na(k)]

idx = as.character(sort(unique(as.integer(c(id[i], k, id[j])))))

foo = read.table("~/DSIProjects/SAX2CSV/r.csv", sep = "\t", comment.char = "", quote = "", header = FALSE, stringsAsFactors = FALSE)
foo[[1]] = gsub('"', '', foo[[1]])

zz = setdiff(idx, foo[[1]])

zz2 = setdiff(foo[[1]], idx)

all.equal()


