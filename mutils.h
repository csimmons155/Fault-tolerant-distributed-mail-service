////////////////////////////////////////////////////////////////////////////////
//
//    James D. Browne
//    Chris Simmons
//    Distributed Assignment 4
//
//    mutils.h
//    Header file for definitions and functions required by client.c and
//    server.c
//    Date: 2016/11/15
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <setjmp.h>

//header type defs, from client
#define	REQ_DEL 	2
#define REQ_HEAD	3
#define REQ_MSG		4
#define REQ_SEND	7
#define REQ_MEM		8

//header type defs, to client
#define SEND_HEAD	3
#define SEND_MSG	4
#define SEND_MEM	8

//message field lengths
#define LEN_USER	10
#define	LEN_SUB		80
#define LEN_MSG		1000

//server group names, defined to prevent FFing.
#define BS1			"BS1"
#define BS2			"BS2"
#define BS3			"BS3"
#define BS4			"BS4"
#define BS5			"BS5"

#define int32u unsigned int

////////////////////////////////////////////////////////////////////////////////
//Linked list to hold user's messages.
////////////////////////////////////////////////////////////////////////////////
typedef struct email_list {
char to_name[LEN_USER];//not sure this is needed.
char from_name[LEN_USER];
int read; //read equals zero if message is unread.
int rec_srv; //server that origionally received the message from a client.
int msg_id; //msg id assigned to msg from rec_srv.
char subject[LEN_SUB];
char msg[LEN_MSG];
	struct email_list *next_email;
} email_list;

////////////////////////////////////////////////////////////////////////////////
//Linked list to hold user's list of headers.
////////////////////////////////////////////////////////////////////////////////
typedef struct head_list {
char from_name[LEN_USER];
int rec_srv;
int msg_id;
char subject[LEN_SUB];
struct head_list *next_head;
} head_list;

////////////////////////////////////////////////////////////////////////////////
//Linked list to hold user's name and list of emails.
////////////////////////////////////////////////////////////////////////////////
typedef struct user_list {
char user_name[LEN_USER];
struct email_list *next_email;
	struct user_list *next_user;
} user_list;

//Define array of server groups
char server_list[5][4] = {BS1, BS2, BS3, BS4, BS5};//put four to include null.  Check this when working.

//Define functions used by both client and server
//read_message();

////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//  11/15/2016 JB: added basic definitions and structures.
//
//
////////////////////////////////////////////////////////////////////////////////
