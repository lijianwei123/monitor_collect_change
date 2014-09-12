CC = gcc 
CFLAGS = -g -Wall -DDEBUG -fPIC -Wl,-rpath,/usr/local/libevent-2.0.21-stable/lib  \
-L/usr/local/libevent-2.0.21-stable/lib \
-L/usr/lib/mysql \
-Llib \
-Wl,-dn -liniparser \
-Wl,-dy  -levent -lcurl -lm  -lmysqlclient \
-I/usr/local/libevent-2.0.21-stable/include

TARGET = look

SRCS = look.c\
	mysql.c\
	helper.c\
	curl_http.c\
	../cJSON.c\
	controller/collect.c

OBJS = $(SRCS:.c=.o)


$(TARGET): $(SRCS)
	$(CC) $^ $(CFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) -o $@ -c $<

clean:
	rm -rf *.o $(TARGET)

.PHONY: clean
