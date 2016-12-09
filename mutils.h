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
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <setjmp.h>
#include "sp.h"

//header type defs, from client
#define	REQ_DEL 	2
#define REQ_HEAD	3
#define REQ_MSG		4
#define REQ_SEND	7
#define REQ_MEM		8
#define REQ_JOIN	9
#define REQ_LEV		10

//header type defs, to client
#define SEND_HEAD	5
#define SEND_NOHD	11
#define SEND_MSG	6
#define SEND_NOMSG	12
#define SEND_MEM	1

//header type defs,to servers
#define UPD_READ	20
#define UPD_DEL		21
#define UPD_MSG		22
#define VIEW		23

//message field lengths
#define LEN_USER	10
#define	LEN_SUB		80
#define LEN_MSG		1000
#define LEN_TOT		1104
#define MAX_MSG		1400

/////////////////////////////
#define SUB_PER		13
#define MAX_DIS		10
#define UPD_SCHD	100
//server group names, defined to prevent FFing.
#define BS1			"BS1"
#define BS2			"BS2"
#define BS3			"BS3"
#define BS4			"BS4"
#define BS5			"BS5"
#define ALL_SVR		"ALL_SVR"
#define ALL_TEST	"ALL_TEST"
#define SVR_PRES	"ALL_PRES"

#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100

#define int32u unsigned int

////////////////////////////////////////////////////////////////////////////////
//Linked list to hold user's messages.
////////////////////////////////////////////////////////////////////////////////
typedef struct email_list {
	char to_name[LEN_USER];//not sure this is needed.
	char from_name[LEN_USER];
	int read; //read equals zero if message is unread.
	int rec_svr; //server that origionally received the message from a client.
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
	int read;
	int rec_svr;
	int msg_id;
	char subject[LEN_SUB];
	struct head_list *next_head;
} head_list;

////////////////////////////////////////////////////////////////////////////////
//Linked list to hold user's name and list of emails.
////////////////////////////////////////////////////////////////////////////////
typedef struct user_list {
	char user_name[LEN_USER];
	struct email_list *head_email;
	struct email_list *tail_email;
	struct user_list *next_user;
} user_list;

///////////////////////////////////////////////////////////////////////////////
//Linked lists to hold updates.  Three types: msg, del, read.
////////////////////////////////////////////////////////////////////////////////
typedef struct update_list {
	int update_type;
	int rec_svr; //server that origionally received the message from a client.
	int msg_id; //msg id assigned to msg from rec_srv.
	char buffer[MAX_MSG];
	struct update_list *next_update;
} update_list;

//Define functions to work with structs.
void add_message(char* buffer);
void del_message(char* msg);
void print_msgs();
void send_header(mailbox Mbox, char* buffer);
int req_message(mailbox Mbox, char* buffer);
void set_proc(int proc_id);
void set_mbox(mailbox Mbox);
void read_file();
void merge_view(mailbox Mbox, char *buffer, int *servers, int num_servers);
void send_view();
void u_email_w(char *msg);
void u_del_w(char* msg);
void u_read_w(char* msg);
void add_update_list(int update_type, char *buffer);
void update_message(char *buffer);
void update_delete(char* msg);
void update_read(char* msg);
void write_update(char *buffer);
void list_update(char *buffer);
int check_update(char *buffer);
void reset_svr_upd_received();
void periodic_mat();

////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//  11/15/2016 JB: added basic definitions and structures.
//
//
////////////////////////////////////////////////////////////////////////////////
