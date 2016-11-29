////////////////////////////////////////////////////////////////////////////////
//
//    James D. Browne
//    Chris Simmons
//    Distributed Assignment 4
//
//    m_server.c
//    This is the server program for the 4th Dist Assignment.
//
//    Date: 2016/11/15
//
////////////////////////////////////////////////////////////////////////////////
#include "sp.h"
#include "mutils.h"

static  char    Spread_name[80];

static  char    Private_group[MAX_GROUP_NAME];
char			join_grp[MAX_GROUP_NAME];//used in Process:REQ_JOIN, REQ_LEV
static  mailbox Mbox;
int 			num_in_group = 0;
int 			total_rec = 0;
int				mid = 0;
static  int     To_exit = 0;
static 	FILE 	*fw;
user_list*		head_user = NULL;
int		server_id;
char server_list[5][4] = {BS1, BS2, BS3, BS4, BS5};//put four to include null.  Check this when working.

/*
#define MAX_MESSLEN     102400
//#define MAX_VSSETS      10
#define MAX_MEMBERS     100
#define MAX_MSG_SEND	10
#define MSG_LEN			304
#define P_IDX			0
#define M_IDX			1
#define R_NUM			2
*/
int start_proc();
void send_mess(int mess_ind, int proc_ind, int num_to_send);
void rec_mess(int num_to_rec);
void Print_help();
void Bye();
void initFile(int procIDX);
void process_message(char* msg);

//////////////////////////////////////////////////////////////////////////////
//MAIN
//////////////////////////////////////////////////////////////////////////////
int main( int argc, char *argv[] )
{
	////////////////Needed to receive messages./////////////////////////
	static	char		 mess[MAX_MESSLEN];
	char		 sender[MAX_GROUP_NAME];
	char		 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	membership_info  memb_info;
	vs_set_info      vssets[MAX_VSSETS];
	unsigned int     my_vsset_index;
	int              num_vs_sets;
	char             members[MAX_MEMBERS][MAX_GROUP_NAME];
	int		 num_groups;
	int		 service_type;
	int16		 mess_type;
	int		 endian_mismatch;
	int		 i,j;
	////////////////////////////////////////////////////////////////////

	int	    ret;

	//get command line arguments.
	if (argc == 2)
	{
		server_id = (int) strtol (argv[1], NULL, 0);
		server_id--;
	}else
	{
		Print_help();
	}

	//before connecting to spread, read info from file to regain state.
	//initFile(server_id);

	//connect to spread.
	sp_time test_timeout;
	test_timeout.sec = 5;
	test_timeout.usec = 0;
	sprintf( Spread_name, "4803");
	ret = SP_connect_timeout( Spread_name, server_list[server_id], 0, 1, &Mbox, Private_group, test_timeout );
	if( ret != ACCEPT_SESSION )
	{
		SP_error( ret );
		Bye();
	}
	printf("connected to server.\n");

	//Join required mailboxes.
	SP_join ( Mbox, server_list[server_id]);
	SP_join ( Mbox, ALL_SVR);
	SP_join ( Mbox, ALL_TEST);
	printf("joined groups.\n");

	while(1)
	{
		service_type = 0;

		ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups,
				&mess_type, &endian_mismatch, sizeof(mess), mess );
		printf("\n============================\n");

		if( ret < 0 )
		{
			if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
				service_type = DROP_RECV;
				printf("\n========Buffers or Groups too Short=======\n");
				ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups,
						&mess_type, &endian_mismatch, sizeof(mess), mess );
			}
		}
		if (ret < 0 )
		{
			if( ! To_exit )
			{
				SP_error( ret );
				printf("\n============================\n");
				printf("\nBye.\n");
			}
			exit( 0 );
		}
		if( Is_regular_mess( service_type ) )
		{
			mess[ret] = 0;
			if     ( Is_unreliable_mess( service_type ) ) printf("received UNRELIABLE ");
			else if( Is_reliable_mess(   service_type ) ) printf("received RELIABLE ");
			else if( Is_fifo_mess(       service_type ) ) printf("received FIFO ");
			else if( Is_causal_mess(     service_type ) ) printf("received CAUSAL ");
			else if( Is_agreed_mess(     service_type ) ) printf("received AGREED ");
			else if( Is_safe_mess(       service_type ) ) printf("received SAFE ");
			printf("message from %s, of type %d, (endian %d) to %d groups \n(%d bytes): %s\n",
					sender, mess_type, endian_mismatch, num_groups, ret, mess );
			process_message(mess);
		}else if( Is_membership_mess( service_type ) )
		{
			ret = SP_get_memb_info( mess, service_type, &memb_info );
			if (ret < 0) {
				printf("BUG: membership message does not have valid body\n");
				SP_error( ret );
				exit( 1 );
			}
			if     ( Is_reg_memb_mess( service_type ) )
			{
				printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
						sender, num_groups, mess_type );//Delete when fully implemented.
				for( i=0; i < num_groups; i++ )
					printf("\t%s\n", &target_groups[i][0] );
				printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );

				if( Is_caused_join_mess( service_type ) )
				{
					printf("Due to the JOIN of %s\n", memb_info.changed_member );
				}else if( Is_caused_leave_mess( service_type ) ){
					printf("Due to the LEAVE of %s\n", memb_info.changed_member );
				}else if( Is_caused_disconnect_mess( service_type ) ){
					printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
				}else if( Is_caused_network_mess( service_type ) ){
					printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
					num_vs_sets = SP_get_vs_sets_info( mess, &vssets[0], MAX_VSSETS, &my_vsset_index );
					if (num_vs_sets < 0) {
						printf("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
						SP_error( num_vs_sets );
						exit( 1 );
					}
					for( i = 0; i < num_vs_sets; i++ )
					{
						printf("%s VS set %d has %u members:\n",
								(i  == my_vsset_index) ?
								("LOCAL") : ("OTHER"), i, vssets[i].num_members );
						ret = SP_get_vs_set_members(mess, &vssets[i], members, MAX_MEMBERS);
						if (ret < 0) {
							printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
							SP_error( ret );
							exit( 1 );
						}
						for( j = 0; j < vssets[i].num_members; j++ )
							printf("\t%s\n", members[j] );
					}
				}
			}else if( Is_transition_mess(   service_type ) ) {
				printf("received TRANSITIONAL membership for group %s\n", sender );
			}else if( Is_caused_leave_mess( service_type ) ){
				printf("received membership message that left group %s\n", sender );
			}else printf("received incorrecty membership message of type 0x%x\n", service_type );
		} else if ( Is_reject_mess( service_type ) )
		{
			printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
					sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
		}else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);

		fflush(stdout);
	}

	/*
	//Wait for processes before starting.
	while(num_in_group != num_of_processes)
	{
	num_in_group = start_proc();
	}
	//Start process
	while (num_in_group != 0)
	{
	//even number processes send first
	if (process_index%2 == 0){
	if (num_of_messages >= num_sent)
	{
	for(int m = 0; m < MAX_MSG_SEND; m++)
	{
	if (num_of_messages >= num_sent)
	{
	num_sent++;
	send_mess(num_sent, process_index, num_of_messages - num_sent);
	}else
	{
	break;
	}
	}
	}
	rec_mess(MAX_MSG_SEND*num_in_group);
//odd number processes receive first
}else
{
rec_mess(MAX_MSG_SEND*num_in_group);
if (num_of_messages >= num_sent)
{
for(int m = 0; m < MAX_MSG_SEND; m++)
{
if (num_of_messages >= num_sent)
{
num_sent++;
send_mess(num_sent, process_index, num_of_messages - num_sent);
}else
{
break;
}
}
}

}
}
*/
Bye();
return( 0 );
}
/*
//////////////////////////////////////////////////////////////////////
//Listen for join messages and return how many processes are in the group.
/////////////////////////////////////////////////////////////////////
int start_proc()
{
char		 mess[MAX_MESSLEN];
char		 sender[MAX_GROUP_NAME];
char		 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
membership_info  memb_info;
int			 num_groups;
int			 service_type;
int16		 mess_type;
int			 endian_mismatch;

service_type = 0;
printf("getting member join messages\n");
SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups,
&mess_type, &endian_mismatch, sizeof(mess), mess );
if( Is_membership_mess( service_type ) )
{
SP_get_memb_info( mess, service_type, &memb_info );
if     ( Is_reg_memb_mess( service_type ) )
{
return num_groups;
}else
{
Bye();
}
}
Bye();
return 0;
}

//////////////////////////////////////////////////////////////////////////
//Send a message as described in the exercise handout.
/////////////////////////////////////////////////////////////////////////
void send_mess(int mess_ind, int proc_ind, int num_to_send)
{
char	groups[MAX_GROUP_NAME];
strcpy(groups, "Browne");
int	num_groups=1;

int mess[MSG_LEN];
//int		 ret;

mess[P_IDX] = proc_ind;
mess[M_IDX] = mess_ind;
mess[R_NUM] = rand() % 1000000 + 1;
//if num_to_send less than 0 then let all processes know this processes is
//complete
if (num_to_send < 0)
{
mess[3] = 0;
} else
{
//normal message, more messages to follow.
mess[3] = 1;
}

SP_multigroup_multicast( Mbox, CAUSAL_MESS, num_groups, (const char (*)[MAX_GROUP_NAME]) groups, 1, MSG_LEN * sizeof(int), (char *)mess );
//printf("sent the message\n");
}
*/
//////////////////////////////////////////////////////////////////////////
//Print the usage message.
/////////////////////////////////////////////////////////////////////////
void    Print_help()
{
	printf( "Usage: m_server <server number 1:5>\n");
	exit( 0 );
}

///////////////////////////////////////////////////////////////////////////
//Disconnect Mbox and quit.
///////////////////////////////////////////////////////////////////////////
void	Bye()
{
	To_exit = 1;

	printf("\nBye.\n");
	fclose(fw);
	SP_disconnect( Mbox );

	exit( 0 );
}
/*
/////////////////////////////////////////////////////////////////////////////////////////////////////
//Receive a message and print contents to file.
/////////////////////////////////////////////////////////////////////////////////
void rec_mess(int num_to_rec)
{
int	num_groups;

int mess[MSG_LEN];
char		 sender[MAX_GROUP_NAME];
char		 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
int		 service_type =0;
int16		 mess_type;
int		 endian_mismatch;
//int		 ret;

for (int i = 0; i < num_to_rec; i++)
{
SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups,
&mess_type, &endian_mismatch, MSG_LEN*sizeof(int), (char *)mess );
if( Is_regular_mess( service_type ) )
{
//	printf("received the right type of message, type %d.\n", service_type);
if (mess[3] == 0)
{
//A process finished.  Reduce number of sending processes.
num_in_group--;
return;
}else
{
fprintf(fw, "%2d, %8d, %8d\n", mess[P_IDX], mess[M_IDX], mess[R_NUM]);
total_rec++;
}
}else
{
printf("received a non routine message of type: %d.\n", service_type);
i--;
}
}
}

///////////////////////////////////////////////////////////////////////////
//Open the file to write output.
///////////////////////////////////////////////////////////////////////////
void initFile(int procIDX)
{
char filename[] = "x.out";
filename[0] = procIDX + '0';
if ((fw = fopen (filename, "w+")) == NULL)
{
perror ("fopen");
exit (0);
}
}
*/

///////////////////////////////////////////////////////////////////////////
//process regular message.
///////////////////////////////////////////////////////////////////////////
void process_message(char* msg)
{
	int msg_type = ((int*) msg)[0];
	printf("message received of type %d", msg_type);

	switch(msg_type)
	{
		case REQ_JOIN:
			strcpy(join_grp , msg+sizeof(int));
			SP_join ( Mbox, join_grp);
			break;
		case REQ_LEV:
			strcpy(join_grp , msg+sizeof(int));
			SP_leave ( Mbox, join_grp);
			break;
		case REQ_SEND:
			mid++;
			add_message(mid, server_id, msg+sizeof(int), head_user);
			break;
		default:
			printf("\nUnknown command received.\n");
			break;
	}
}
////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//  11/15/2016 JB: Cleaned up code from project 3 to use in project 4.
//
//
/////////////////////////////////////////////////////////////////////////////////
