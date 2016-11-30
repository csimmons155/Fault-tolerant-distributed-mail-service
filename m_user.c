////////////////////////////////////////////////////////////////////////////////
//
//    James D. Browne
//    Chris Simmons
//    Distributed Assignment 4
//
//    m_user.c
//    This is the client program for the 4th Dist Assignment.
//
//    Date: 2016/11/15
//
////////////////////////////////////////////////////////////////////////////////
#include "sp.h"
#include "mutils.h"

static	char	User[80];
static  char    Spread_name[80];

static  char    Private_group[MAX_GROUP_NAME];
static	char	join_svr_grp[MAX_GROUP_NAME];//connected to server will join this group.
static  mailbox Mbox;
static	int	    Num_sent;

static  int     To_exit = 0;
char server_list[5][4] = {BS1, BS2, BS3, BS4, BS5};//put four to include null.  Check this when working.

static	char	username[LEN_USER + 1];

//char* server_groups[] = { "servg0", "servg1", "servg2", "servg3", "servg4"};
int curr_svr = -1;
int targ_svr = -1;

static	void	Print_menu();
static	void	User_command();
static  void	Bye();
static	void	Read_message();
static	int		Check_user();
static void make_svr_grp(char *priv, char *grp); //make unique server group
void connect_svr();
void print_servers();
void send_email();
void request_list();

int main( int argc, char *argv[] )
{
	int	    ret;

	//Connect to spread.
	sp_time test_timeout;
	test_timeout.sec = 5;
	test_timeout.usec = 0;
	sprintf( Spread_name, "4803");
	User[0] = 0;
	strcpy(username, "User");
	ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group, test_timeout );
	if( ret != ACCEPT_SESSION )
	{
		SP_error( ret );
		Bye();
	}

	// just for testing.  Delete.
	//	printf("User: connected to %s with private group %s\n", Spread_name, Private_group );

	//Create unique server connection group and join the group.
	make_svr_grp(Private_group, join_svr_grp);
	ret = SP_join ( Mbox, join_svr_grp);
	if (ret < 0){
		printf("did not join\n");
		Bye();
	}

	//Attach functions and start handlers.
	E_init();
	E_attach_fd( 0, READ_FD, User_command, 0, NULL, LOW_PRIORITY );
	E_attach_fd( Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY );
	Print_menu();
	E_handle_events();

	return( 0 );
}

static	void	User_command()
{
	char	command[130];
	//char	mess[MAX_MESSLEN];
	//	char	group[80];
	//char	groups[10][MAX_GROUP_NAME];
	//int	num_groups;
	//unsigned int	mess_len;
	int	ret;
	int	i;
	int new_svr;

	for( i=0; i < sizeof(command); i++ ) command[i] = 0;
	if( fgets( command, 130, stdin ) == NULL )
	{
		printf("Unrecognized user command.  Please make your selection again.\n");
		Print_menu();
		return;
	}

	switch( command[0] )
	{
		case 'u'://to login
			ret = sscanf( &command[2], "%10s", username);
			if( ret < 1 )
			{
				printf(" invalid username.\n");
				printf("\n%s> ", username);
				break;
			}
			Print_menu();
			break;

		case 'c': //connect to server
			ret = sscanf( &command[2], "%d", &new_svr);
			new_svr--;//change to index instead of user readable.
			if( ret < 1 || new_svr > 4 || new_svr < 0)
			{
				printf(" invalid server number.  Choose from 1 to 5..\n");
				Print_menu();
				break;
			}
			targ_svr = new_svr;
			//connect_svr();
			SP_join ( Mbox, server_list[targ_svr]);
			SP_leave ( Mbox, server_list[targ_svr]);
			break;

		case 'l':
			if (!Check_user()) break;
			request_list();
			Print_menu();
			break;

		case 'm':
			if (!Check_user()) break;
			//implement m
			send_email();

			Num_sent++;

			Print_menu();
			break;

		case 'd':
			if (!Check_user()) break;
			//implement d
			Print_menu();
			break;

		case 'r':
			if (!Check_user()) break;

			//imp r
			Print_menu();
			break;

		case 'v':
			print_servers();
			Print_menu();
			break;

		case 'q':
			Bye();
			break;

		default:
			printf("\nUnknown commnad\n");
			Print_menu();
			break;
	}
}

static	void	Print_menu()
{
	printf("\n");
	printf("==========\n");
	printf("User Menu:\n");
	printf("----------\n");
	if (curr_svr == -1)
	{
		printf("not connected to a server\n");
	} else
	{
		printf("connected to server %d.\n", curr_svr + 1);
	}
	printf("\n");
	printf("\tu <user name> -- login as user.  Limit 10 chars.\n");
	printf("\tc <server index> -- connect to server\n");
	printf("\tl  -- list messages\n");
	printf("\tm -- follow prompts to send message\n");
	printf("\td <list number> -- delete a message\n");
	printf("\tr <list number> -- read a message (stuck) \n");
	printf("\tv -- print server membership \n");
	printf("\tq -- quit\n");

	printf("\n%s> ", username);
	fflush(stdout);
}

/* FIXME: The user.c code does not use memcpy()s to avoid bus errors when
 *        dereferencing a pointer into a potentially misaligned buffer */

static	void	Read_message()
{

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
	int		 ret;

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
				//added for server connect.///////////////////////////////////
				if (!strcmp(sender, server_list[targ_svr])){//check if server is in its group.
					for( i=0; i < num_groups; i++ )
					{
						printf("%3s compared to %s\n", &target_groups[i][1], server_list[targ_svr]);
						if (!strncmp( &target_groups[i][1], server_list[targ_svr], 3))
						{
							connect_svr();//if server is in its group then send connect message.
							break;
						}
						if (i == num_groups -1)
						{
							printf("Unable to connect to server %d.\n", targ_svr + 1);
						}
					}
				}
				////////////////////////////////////////////////////////////
				printf("Due to the JOIN of %s\n", memb_info.changed_member );
			}else if( Is_caused_leave_mess( service_type ) ){
				//added for server connect.///////////////////////////////////
				if (!strcmp(sender, join_svr_grp))//if server left group then send disconnect.
				{
					if (num_groups == 1)
					{
						curr_svr = -1;
						if (targ_svr == -1){
							printf("disconnected from server\n");
							Print_menu();
						}
					}
				}
				////////////////////////////////////////////////////////////
				printf("Due to the LEAVE of %s\n", memb_info.changed_member );
			}else if( Is_caused_disconnect_mess( service_type ) ){
				//added for server connect.///////////////////////////////////
				if (!strcmp(sender, join_svr_grp))//if server left group then send disconnect.
				{
					if (num_groups == 1)
					{
						curr_svr = -1;
						if (targ_svr == -1){
							printf("disconnected from server\n");
							Print_menu();
						}
					}
				}
				////////////////////////////////////////////////////////////

				printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
			}else if( Is_caused_network_mess( service_type ) ){
				//added for server connect.///////////////////////////////////
				if (!strcmp(sender, join_svr_grp))//if server left group then send disconnect.
				{
					if (num_groups == 1)
					{
						curr_svr = -1;
						if (targ_svr == -1){
							printf("disconnected from server\n");
							Print_menu();
						}
					}
				}
				////////////////////////////////////////////////////////////

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
}

////////////////////////////////////////////////////////////////////////////////////////
//Exit client
////////////////////////////////////////////////////////////////////////////////////////
static  void	Bye()
{
	To_exit = 1;

	printf("\nBye.\n");

	SP_disconnect( Mbox );

	exit( 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
//Makes sure the username is set and a server is connected to.  Otherwise
//returns 0
/////////////////////////////////////////////////////////////////////////////////////////
static	int		Check_user(){
	char	uname[5] = "User";
	int		ret = 1;
	if (!strncmp(uname, username, 5))
	{
		ret= 0;
		printf("Change username before performing this function.\n");
	}
	if (curr_svr == -1)
	{
		printf("Connect to a server before performing this function.\n");
		ret= 0;
	}
	if (ret == 0 )
	{
		Print_menu();
	}
	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
//Create the server connection group.  This group is unique to the client.
/////////////////////////////////////////////////////////////////////////////////////////
static void make_svr_grp(char *priv, char *grp)
{
	strcpy(grp, priv);
	grp[0] = 's';
	grp[8] = 's';
	grp[9] = 's';
}

////////////////////////////////////////////////////////////////////////////////////////
//Connect to a server and disconnect from previous server.  Join target server's
//group and see if the server is present.  Leave target server's group.  If server
//exists, connect to target server and dissconnect from
//old.  Otherwise, stay connected to old server and inform user the server is
//not available.
////////////////////////////////////////////////////////////////////////////////////////
void connect_svr()
{
	char	groups[MAX_GROUP_NAME];
	int msg_type;
	int	num_groups=1;
	char mess[MAX_GROUP_NAME + sizeof(int)];
	int svr_num = targ_svr;

	//join new server and unjoin previous server.
	msg_type = REQ_JOIN;
	strcpy(groups, server_list[svr_num] );
	memcpy(mess, &msg_type, sizeof(int));
	strcpy(mess+sizeof(int), join_svr_grp);

	SP_multigroup_multicast( Mbox, CAUSAL_MESS, num_groups, (const char (*)[MAX_GROUP_NAME]) groups, 1, MAX_GROUP_NAME + sizeof(int), mess );

	if (curr_svr != -1){
		msg_type = REQ_LEV;
		strcpy(groups, server_list[svr_num] );
		memcpy(mess, &msg_type, sizeof(int));
		strcpy(mess+sizeof(int), join_svr_grp);

		SP_multigroup_multicast( Mbox, CAUSAL_MESS, num_groups, (const char (*)[MAX_GROUP_NAME]) groups, 1, MAX_GROUP_NAME + sizeof(int), mess );
	}

	curr_svr = targ_svr;
	targ_svr = -1;
}

////////////////////////////////////////////////////////////////////////////////
//Print servers in current group.
///////////////////////////////////////////////////////////////////////////////
void print_servers()
{

}

//////////////////////////////////////////////////////////////////////////////
//Send an email.
/////////////////////////////////////////////////////////////////////////////
void send_email()
{
	char to_user[LEN_USER];
	char subject[LEN_SUB];
	char email[LEN_MSG];
	//int total_length = 0;
	int msg_type = REQ_SEND;
	int mess_len;
	char msg[LEN_TOT];
	int ret;

	printf("\nto: ");
	mess_len = 0;
	while(mess_len == 0)
	{
		if (fgets(&to_user[mess_len], LEN_USER, stdin) == NULL)
			Bye();

		mess_len += strlen( &to_user[mess_len] );
		if (mess_len == 0)
		{
			printf("This field cannot be blank\n");
			printf("\nto: ");
		}
	}
to_user[mess_len-1] = 0;

	printf("\nsubject: ");
	mess_len = 0;
	while ( mess_len == 0) {
		if (fgets(&subject[mess_len], LEN_SUB, stdin) == NULL)
			Bye();
		mess_len += strlen( &subject[mess_len] );
		if (mess_len == 0)
		{
			printf("This field cannot be blank\n");
			printf("\nsubject: ");
		}
	}
	subject[mess_len-1] = 0;

	printf("\nemail: ");
	mess_len = 0;
	while ( mess_len == 0) {
		if (fgets(&email[mess_len], LEN_MSG, stdin) == NULL)
			Bye();
		mess_len += strlen( &email[mess_len] );
		if (mess_len == 0)
		{
			printf("This field cannot be blank\n");
			printf("\nemail: ");
		}
	}
	email[mess_len-1]= 0;

	int runner = 0;
	memcpy(msg, &msg_type, sizeof(int));
	runner += sizeof(int);
	memcpy(msg+runner, to_user,LEN_USER);
	runner += LEN_USER;
	memcpy(msg+runner, username,LEN_USER);
	runner += LEN_USER;
	memcpy(msg+runner, subject, LEN_SUB);
	runner += LEN_SUB;
	memcpy(msg+runner, email, LEN_MSG);
	runner += LEN_MSG;

	ret= SP_multicast( Mbox, SAFE_MESS, server_list[curr_svr], 1, LEN_TOT, msg );
	if( ret < 0 )
	{
		SP_error( ret );
		Bye();
	}
}

////////////////////////////////////////////////////////////////////////
//Request a list of headers from the server.
////////////////////////////////////////////////////////////////////////
void request_list()
{
	int runner = 0;
	int msg_type = REQ_HEAD ;
	int ret;
	char msg[sizeof(int) + LEN_USER + MAX_GROUP_NAME];

	memcpy(msg, &msg_type, sizeof(int));
	runner += sizeof(int);
	memcpy(msg+runner, username,LEN_USER);
	runner += LEN_USER;
	strcpy(msg+runner, Private_group);
	runner += MAX_GROUP_NAME;

	ret= SP_multicast( Mbox, SAFE_MESS, server_list[curr_svr], 1, runner, msg );
	if( ret < 0 )
	{
		SP_error( ret );
		Bye();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//  11/15/2016 JB: Cleaned up code from project 3 to use in project 4.
//  11/26/2016 JB: Added username functionality.
//
////////////////////////////////////////////////////////////////////////////////
