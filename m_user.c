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

char	User[80];
char    Spread_name[80];

char    Private_group[MAX_GROUP_NAME];
char	join_svr_grp[MAX_GROUP_NAME];//connected to server will join this group.
char 	error_msg[40] = "";//Used to display error messages on menu.
mailbox Mbox;
int	    Num_sent;

int     To_exit = 0;
char 	server_list[5][4] = {BS1, BS2, BS3, BS4, BS5};//A list of the servers' names.

char	username[LEN_USER + 1];

int 	curr_svr = -1;
int 	targ_svr = -1;

//Following variables are used to track subject list packets
int 	tot_exp = 0;//total number of packets expected for list.
int 	tot_rec = 0;//number of packets received for list.
int 	tot_subs = 0;//total number of subs received.
head_list* subs = NULL;//Used to store list of subjects to display.
int 	displaying = 0;//first sub being displayed.

int 	user_mode = 1;//user mode one is normal, usermode 2 means in list mode.

void	Print_menu();
void	User_command();
void	Bye();
void	Read_message();
int		Check_user();
void 	make_svr_grp(char *priv, char *grp); //make unique server group
void 	connect_svr();
void	print_servers();
void 	send_email();
void 	request_list();
void 	norm_rec(char* msg);
void 	process_headers(char* msg);
void 	show_header();
void	free_emails();
void	delete_email(int email_num);
void 	req_email(int email_num);
void 	display_msg();

int main( int argc, char *argv[] )
{
	int	    ret;

	//Connect to spread.
	sp_time test_timeout;
	test_timeout.sec = 5;
	test_timeout.usec = 0;
	sprintf( Spread_name, "9625");
	User[0] = 0;
	strcpy(username, "User");
	ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group, test_timeout );
	if( ret != ACCEPT_SESSION )
	{
		SP_error( ret );
		Bye();
	}

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

void	User_command()
{
	char	command[130];
	int	ret;
	int	i;
	int new_svr;
	int tar_email;
	int curr_view;

	if (user_mode == 1){
		for( i=0; i < sizeof(command); i++ ) command[i] = 0;
		if( fgets( command, 130, stdin ) == NULL )
		{
			strcpy(error_msg, "Unrecognized user command.");
			Print_menu();
			return;
		}

		switch( command[0] )
		{
			case 'u'://to login
				ret = sscanf( &command[2], "%10s", username);
				if( ret < 1 )
				{
					strcpy(error_msg, "invalid username.");
					Print_menu();
					break;
				}
				Print_menu();
				break;

			case 'c': //connect to server
				ret = sscanf( &command[2], "%d", &new_svr);
				new_svr--;//change to index instead of user readable.
				if( ret < 1 || new_svr > 4 || new_svr < 0)
				{
					strcpy(error_msg, "invalid server number.  Choose from 1 to 5.");
					Print_menu();
					break;
				}
				targ_svr = new_svr;
				SP_join ( Mbox, SVR_PRES);
				SP_leave ( Mbox, SVR_PRES);
				strcpy(error_msg, "Server is not Connecting, try again.");
				Print_menu();
				break;

			case 'l':
				if (!Check_user()) break;
				request_list();
				Print_menu();
				break;

			case 'm':// send a message
				if (!Check_user()) break;
				send_email();
				Num_sent++;
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
				strcpy(error_msg, "Unknown command.");
				Print_menu();
				break;
		}
	}else if (user_mode == 2){
		for( i=0; i < sizeof(command); i++ ) command[i] = 0;
		if( fgets( command, 130, stdin ) == NULL )
		{
			strcpy(error_msg, "Unrecognized user command.  Please make your selection again.");
			printf("Unrecognized user command.  Please make your selection again.\n");
			Print_menu();
			return;
		}

		switch( command[0] )
		{
			case 'r'://display message
				ret = sscanf( &command[2], "%d", &tar_email);
				curr_view = tot_subs - (displaying)*MAX_DIS;
				if(curr_view > MAX_DIS)
				{
					curr_view = MAX_DIS;
				}

				if (tar_email < 1 || tar_email > curr_view)
				{
					strcpy(error_msg, "value selected unavailable.  Please choose a correct value.");
					show_header();

				}else
				{
					req_email(tar_email);
				}
				break;

			case 'd': //delete message

				ret = sscanf( &command[2], "%d", &tar_email);

				curr_view = tot_subs - (displaying)*MAX_DIS;

				if(curr_view > MAX_DIS)
				{
					curr_view = MAX_DIS;
				}

				if (tar_email < 1 || tar_email > curr_view)
				{
					strcpy(error_msg, "value selected unavailable.  Please choose a correct value.");
				}else
				{
					delete_email(tar_email);
				}

				break;

			case 'n'://next 10 emails
				if ((displaying+1)*MAX_DIS>=tot_subs){
					strcpy(error_msg, "Already at end of list.");
				}else
				{
					displaying++;
				}
				show_header();
				break;

			case 'p'://prev 10 emails
				if(displaying == 0)
				{
					strcpy(error_msg, "Already at beginning of list.");
				}else
				{
					displaying--;
				}
				show_header();

				break;

			case 'q'://go back to normal menu.
				free_emails();
				Print_menu();

				break;

			default:
				strcpy(error_msg, "Unknown commnad.");
				show_header();
				break;
		}
	}else if (user_mode == 3)
	{
		getchar();
		user_mode = 2;
		free_emails();
		request_list();
	}else
	{
		strcpy(error_msg, "error - should never get here.");
	}

}
void	Print_menu()
{
	system("clear");
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
	printf("\tv -- print server membership \n");
	printf("\tq -- quit\n");

	printf("\n%s\n", error_msg);
	strcpy(error_msg, "");
	printf("%s> ", username);
	fflush(stdout);
}

void	Read_message()
{

	char		 mess[MAX_MESSLEN];
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
	int		 i;
	int		 ret;

	service_type = 0;

	ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups,
			&mess_type, &endian_mismatch, sizeof(mess), mess );

	if( ret < 0 )
	{
		if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
			service_type = DROP_RECV;
			ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups,	&mess_type, &endian_mismatch, sizeof(mess), mess );
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
		//////////////////////////////////////////////////////////
		//added to handle rec email and rec list.
		//////////////////////////////////////////////////////////
		norm_rec(mess);
		/////////////////////////////////////////////////////////
		mess[ret] = 0;
	}else if( Is_membership_mess( service_type ) )
	{
		free_emails();
		ret = SP_get_memb_info( mess, service_type, &memb_info );
		if (ret < 0) {
			printf("BUG: membership message does not have valid body\n");
			SP_error( ret );
			exit( 1 );
		}
		if     ( Is_reg_memb_mess( service_type ) )
		{
			for( i=0; i < num_groups; i++ )
				if( Is_caused_join_mess( service_type ) )
				{
					//added for server connect.///////////////////////////////////
					if (!strcmp(sender, SVR_PRES)){//check if server is in its group.
						for( i=0; i < num_groups; i++ )
						{
							if (!strncmp( &target_groups[i][1], server_list[targ_svr], 3))
							{
								strcpy(error_msg, "Connecting to server.");
								connect_svr();//if server is in its group then send connect message.
								Print_menu();
								break;
							}
							if (i == num_groups -1)
							{
								strcpy(error_msg, "Chosen server currently not available.");
							}
						}
					}else if (!strcmp(sender, ALL_TEST))//determine what servers are visible.
					{
						ret = 0;
						system("clear");
						printf("servers in group-\n");
						for( i=0; i < num_groups; i++ )
						{
							if (!strncmp( &target_groups[i][1], "BS", 2))
							{
								ret = 1;
								printf("%s\n", &target_groups[i][1]);
							}
						}
						if (!ret)
						{
							printf("none\n");
						}
						printf("******************\n");
						printf("Press [Enter] to continue\n");
						getchar();
						Print_menu();
					}
				}else if( Is_caused_leave_mess( service_type ) ){
					//added for server disconnect.///////////////////////////////////
					if (!strcmp(sender, join_svr_grp))//if server left group then send disconnect.
					{
						if (num_groups == 1)
						{
							curr_svr = -1;
							if (targ_svr == -1){
								strcpy(error_msg, "disconnected from server");
								Print_menu();
							}
						}
					}
				}else if( Is_caused_disconnect_mess( service_type ) ){
					if (!strcmp(sender, join_svr_grp))//if server left group then send disconnect.
					{
						if (num_groups == 1)
						{
							curr_svr = -1;
							if (targ_svr == -1){

								strcpy(error_msg, "disconnected from server");
								Print_menu();
							}
						}
					}
				}else if( Is_caused_network_mess( service_type ) ){
					if (!strcmp(sender, join_svr_grp))//if server left group then send disconnect.
					{
						if (num_groups == 1)
						{
							curr_svr = -1;
							if (targ_svr == -1){

								strcpy(error_msg, "disconnected from server");
								Print_menu();
							}
						}
					}
					num_vs_sets = SP_get_vs_sets_info( mess, &vssets[0], MAX_VSSETS, &my_vsset_index );
					if (num_vs_sets < 0) {
						printf("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
						SP_error( num_vs_sets );
						exit( 1 );
					}
					for( i = 0; i < num_vs_sets; i++ )
					{
						ret = SP_get_vs_set_members(mess, &vssets[i], members, MAX_MEMBERS);
						if (ret < 0) {
							printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
							SP_error( ret );
							exit( 1 );
						}
					}
				}
		}
	} else if ( Is_reject_mess( service_type ) )
	{
		free_emails();
	}else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
}

////////////////////////////////////////////////////////////////////////////////////////
//Exit client
////////////////////////////////////////////////////////////////////////////////////////
void	Bye()
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
int		Check_user(){
	char	uname[5] = "User";
	int		ret = 1;
	if (!strncmp(uname, username, 5))
	{
		ret= 0;
		strcpy(error_msg, "Change username before performing this function.");
	}
	if (curr_svr == -1)
	{
		strcpy(error_msg, "Connect to a server before performing this function.");
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
void make_svr_grp(char *priv, char *grp)
{
	//Server group is made by changing some characters in the user's private
	//group name.
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
	//	printf("trying to join server %d.\n", svr_num+1);
	if (curr_svr != -1 ){
		msg_type = REQ_LEV;
		strcpy(groups, server_list[curr_svr] );
		memcpy(mess, &msg_type, sizeof(int));
		strcpy(mess+sizeof(int), join_svr_grp);

		SP_multigroup_multicast( Mbox, CAUSAL_MESS, num_groups, (const char (*)[MAX_GROUP_NAME]) groups, 1, MAX_GROUP_NAME + sizeof(int), mess );
	}
	msg_type = REQ_JOIN;
	strcpy(groups, server_list[svr_num] );
	memcpy(mess, &msg_type, sizeof(int));
	strcpy(mess+sizeof(int), join_svr_grp);

	SP_multigroup_multicast( Mbox, CAUSAL_MESS, num_groups, (const char (*)[MAX_GROUP_NAME]) groups, 1, MAX_GROUP_NAME + sizeof(int), mess );

	curr_svr = targ_svr;
	targ_svr = -1;
}

////////////////////////////////////////////////////////////////////////////////
//Print servers in current group.
///////////////////////////////////////////////////////////////////////////////
void print_servers()
{
	SP_join ( Mbox, ALL_TEST);
	SP_leave ( Mbox, ALL_TEST);
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
	mess_len = 0;

	while(mess_len == 0 || mess_len >= LEN_USER)
	{
		printf("\nto: ");
		if (fgets(&to_user[mess_len], LEN_USER, stdin) == NULL)
			Bye();

		mess_len = strlen(to_user) - 1;
		if (mess_len == 0)
		{
			printf("This field cannot be blank\n");
		}else if (mess_len >= LEN_USER)
		{
printf("This field cannot be more than %d characters.\n", LEN_USER - 1);
		}else
		{
			to_user[mess_len] = 0;
		}
	}

	mess_len = 0;
	while ( mess_len == 0|| mess_len >= LEN_SUB) {
		printf("\nsubject: ");
		if (fgets(&subject[mess_len], LEN_SUB, stdin) == NULL)
			Bye();
		mess_len = strlen(subject) -1;
		if (mess_len == 0)
		{
			printf("This field cannot be blank\n");
		}else if(mess_len >= LEN_SUB)
		{
printf("This field cannot be more than %d characters.\n", LEN_SUB - 1);
		}else
		{
			subject[mess_len]=0;
		}
	}

	mess_len = 0;
	while ( mess_len == 0|| mess_len >= LEN_MSG) {
		printf("\nemail: ");
		if (fgets(&email[mess_len], LEN_MSG, stdin) == NULL)
			Bye();
		mess_len = strlen(email) - 1;
		if (mess_len == 0)
		{
			printf("This field cannot be blank\n");
		}else if(mess_len >= LEN_MSG)
		{
printf("This field cannot be more than %d characters.\n", LEN_MSG - 1);
		}else
		{
			email[mess_len] = 0;
		}
	}

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

	ret= SP_multicast( Mbox, CAUSAL_MESS, server_list[curr_svr], 1, LEN_TOT, msg );
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

	ret= SP_multicast( Mbox, CAUSAL_MESS, server_list[curr_svr], 1, runner, msg );
	if( ret < 0 )
	{
		SP_error( ret );
		Bye();
	}
}

/////////////////////////////////////////////////////////////////////////
//Used to receive messages send directly to this client.  Should only be
//message list, email, and server list; anything else should be ignored.
//////////////////////////////////////////////////////////////////////////
void norm_rec(char* msg)
{
	int msg_type = ((int*) msg)[0];
	//	printf("message received of type %d", msg_type);

	if (msg_type != SEND_HEAD && msg_type != SEND_MSG)
	{
		free_emails();
	}
	switch(msg_type)
	{
		case SEND_HEAD:
			//This is a list of subjects.
			process_headers(msg+sizeof(int));
			break;
		case SEND_NOHD:
			//This is if there are no subjects on the server for the user.
			strcpy(error_msg, "User has no emails to list.");
						Print_menu();
			break;
		case SEND_MSG:

			//this is to display a received email.
			display_msg(msg+sizeof(int));
			break;

		case SEND_NOMSG:
			strcpy(error_msg, "The email no longer exists.");
						free_emails();
			request_list();
			break;
		default:
			strcpy(error_msg, "Unknown Message received12.");
			break;
	}
}

//////////////////////////////////////////////////////////////////
//process the list of sent email headers.
//////////////////////////////////////////////////////////////////
void process_headers(char* msg)
{
	int p_req = ((int*) msg)[0];
	int p_num = ((int*) msg)[1];
	int sub_num = ((int*) msg)[2];
	int temp_read;
	int temp_id;
	int temp_svr;
	int size_read = sizeof(int)*3;
	///////////////////////////
	//keep track across packets using global variables.
	///////////////////////////
	if (tot_rec != p_num){
		printf("err- received unexpected packet of subjects1.\n");
		free_emails();
		return;
	}
	if (tot_rec > 0 && p_req != tot_exp)
	{
		printf("err- received unexpected packet of subjects2.\n");
		free_emails();
		return;
	}
	if (tot_rec == 0)
	{
		tot_exp = p_req;
	}

	tot_rec++;
	///////////////////////////////////////////////

	if (sub_num > SUB_PER)
	{
		sub_num = SUB_PER;
	}
	tot_subs += sub_num;
	head_list* curr = NULL;
	head_list* prev = NULL;

	if (p_num == 0)//This is the first pack for this list.
	{
		subs = malloc(sizeof(head_list));
		sub_num--;
		memcpy(&temp_read, msg+size_read, sizeof(int));
		subs->read = temp_read;
		size_read+=sizeof(int);
		memcpy(&temp_svr, msg+size_read, sizeof(int));
		subs->rec_svr = temp_svr;
		size_read+=sizeof(int);
		memcpy(&temp_id, msg+size_read, sizeof(int));
		subs->msg_id = temp_id;
		size_read+=sizeof(int);
		strncpy(subs->from_name, msg+size_read, LEN_USER);
		size_read+=LEN_USER;
		strncpy(subs->subject, msg+size_read, LEN_SUB);
		size_read+=LEN_SUB;
		subs->next_head = NULL;
		prev = subs;
	}else
	{
		if (subs == NULL)
		{
			printf("the header list is NULL, this should not happen.\n");
			Bye();
		}else
			prev = subs;
		while (prev->next_head != NULL)
		{
			prev = prev->next_head;
		}
	}
	for (int q = 0; q < sub_num; q++)
	{
		curr= malloc(sizeof(head_list));
		memcpy(&temp_read, msg+size_read, sizeof(int));
		curr->read = temp_read;
		size_read+=sizeof(int);
		memcpy(&temp_svr, msg+size_read, sizeof(int));
		curr->rec_svr = temp_svr;
		size_read+=sizeof(int);
		memcpy(&temp_id, msg+size_read, sizeof(int));
		curr->msg_id = temp_id;
		size_read+=sizeof(int);
		strncpy(curr->from_name, msg+size_read, LEN_USER);
		size_read+=LEN_USER;
		strncpy(curr->subject, msg+size_read, LEN_SUB);
		size_read+=LEN_SUB;
		curr->next_head = NULL;

		prev->next_head = curr;
		prev = curr;
	}

	if (tot_rec == tot_exp)
	{
		user_mode = 2;
		show_header();
	}

}

//////////////////////////////////////////////////////////////////////
//Show list of headers to user and get input.
/////////////////////////////////////////////////////////////////////
void show_header()
{
	int num_to_display;
	head_list* curr = subs;

	for(int n = 0; n < displaying*MAX_DIS ; n++)
	{
		curr = curr->next_head;
	}
	//determine how many emails to display.
	num_to_display = tot_subs - (displaying*MAX_DIS);
	if (num_to_display > MAX_DIS)
	{
		num_to_display = MAX_DIS;
	}

	system("clear");
	printf("\n");
	printf("==========\n");
	printf("User Menu:\n");
	printf("----------\n");
	printf("connected to server %d.\n", curr_svr + 1);
	printf("\n   U  From\tSubject\n");

	for(int m = 0; m < num_to_display; m++)
	{
		if(!curr->read)
		{
			printf("%d. X  ", m+1);
		}else
		{
			printf("%d.    ", m+1);
		}
		printf("%s\t%s\t%d\t%d\n", curr->from_name, curr->subject,curr->rec_svr, curr->msg_id );
		curr = curr->next_head;
	}
	printf("\n");
	printf("\td <list number> -- delete a message\n");
	printf("\tr <list number> -- read a message\n");
	printf("\tn -- display next 10 emails.\n");
	printf("\tp -- display previous 10 emails.\n");
	printf("\tq -- return to main menu.\n");

	printf("\n%s\n", error_msg);
	strcpy(error_msg, "");
	printf("\n%s> ", username);
	fflush(stdout);
}

//////////////////////////////////////////////////////////////////////
//Free memory for email list and set list variables to 0.
///////////////////////////////////////////////////////////////////////
void free_emails()
{
	tot_exp = 0;//total number of packets expected for list.
	tot_rec = 0;//number of packets received for list.
	tot_subs = 0;//total number of subs received.

	head_list* curr = subs;

	while (subs != NULL)
	{
		subs = subs->next_head;
		free(curr);
		curr = subs;
	}
	user_mode = 1;
}

/////////////////////////////////////////////////////////////////////////
//loop through list, find email, send delete msg to server and receive list
//again.
//////////////////////////////////////////////////////////////////////////
void delete_email(int email_num)
{
	head_list* curr = subs;
	for(int n = 1; n < displaying*MAX_DIS+email_num ; n++)
	{
		curr = curr->next_head;
	}
	int runner = 0;
	int rec_svr = curr->rec_svr;
	int msg_id = curr->msg_id;
	int msg_type = REQ_DEL ;
	int ret;
	char msg[MAX_MSG];

	memcpy(msg+runner, &msg_type, sizeof(int));
	runner += sizeof(int);
	memcpy(msg+runner, &rec_svr, sizeof(int));
	runner += sizeof(int);
	memcpy(msg+runner, &msg_id, sizeof(int));
	runner += sizeof(int);
	memcpy(msg+runner, username,LEN_USER);
	runner += LEN_USER;
	memcpy(msg+runner, Private_group, MAX_GROUP_NAME);
	runner += MAX_GROUP_NAME;
	ret= SP_multicast( Mbox, CAUSAL_MESS, server_list[curr_svr], 1, runner, msg );
	if( ret < 0 )
	{
		SP_error( ret );
		Bye();
	}
	free_emails();
}

/////////////////////////////////////////////////////////////////////////
//request email from server to display.
////////////////////////////////////////////////////////////////////////
void req_email(int email_num)
{
	head_list* curr = subs;
	for(int n = 1; n < displaying*MAX_DIS+email_num ; n++)
	{
		curr = curr->next_head;
	}
	int runner = 0;
	int rec_svr = curr->rec_svr;
	int msg_id = curr->msg_id;
	int msg_type = REQ_MSG ;
	int ret;
	char msg[MAX_MSG];

	memcpy(msg+runner, &msg_type, sizeof(int));
	runner += sizeof(int);
	memcpy(msg+runner, &rec_svr, sizeof(int));
	runner += sizeof(int);
	memcpy(msg+runner, &msg_id, sizeof(int));
	runner += sizeof(int);
	memcpy(msg+runner, username,LEN_USER);
	runner += LEN_USER;
	memcpy(msg+runner, Private_group, MAX_GROUP_NAME);
	runner += MAX_GROUP_NAME;
	ret= SP_multicast( Mbox, CAUSAL_MESS, join_svr_grp, 1, runner, msg );
	if( ret < 0 )
	{
		SP_error( ret );
		Bye();
	}
	free_emails();
}

////////////////////////////////////////////////////////////
//When a message is received this will display it to the screen.
////////////////////////////////////////////////////////////
void display_msg(char* msg)
{
	user_mode = 3;

	char from_name[LEN_USER];
	char subject[LEN_SUB];
	char email[LEN_MSG];
	int runner;

	runner = 0;
	memcpy(from_name, msg+runner, LEN_USER);
	runner+=LEN_USER;
	memcpy(subject, msg+runner, LEN_SUB);
	runner+=LEN_SUB;
	memcpy(email, msg+runner, LEN_MSG);
	runner+=LEN_MSG;

	system("clear");
	printf("From: %s\n", from_name);
	printf("Subject: %s\n", subject);
	printf("Message: %s\n", email);
	printf("Press [Enter] to continue\n");
}

////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//
////////////////////////////////////////////////////////////////////////////////
