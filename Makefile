XML2_CONFIG=xml2-config
CFLAGS=-g $(shell $(XML2_CONFIG) --cflags)
LIBS=$(shell $(XML2_CONFIG) --libs)

sax2csv: sax2csv.o
	$(CC) -o $@ $^ $(LIBS)


postsForTag: postsForTag.o
	$(CC) -g -o $@ $^ $(LIBS)



Badges.csv: sax2csv
	./sax2csv Badges.xml Badges.csv  Id UserId Name Date

PostHistory.csv: PostHistory.xml sax2csv
	./sax2csv PostHistory.xml PostHistory.csv Id PostHistoryTypeId PostId RevisionGUID  CreationDate UserId Text UserDisplayName Comment

#%.o: %.c
