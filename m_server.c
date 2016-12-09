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
static  int     To_exit = 0;
static 	FILE 	*fw;
//user_list*		head_user = NULL;
int		server_id;
char server_list[5][4] = {BS1, BS2, BS3, BS4, BS5};//put four to include null.  Check this when working.
int servers_present[5] = { 0 };
int num_servers_present = 0;

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
	set_proc(server_id);
	printf("connected to spread server2.\n");

	//before connecting to spread, read info from file to regain state.
	read_file();
	printf("connected to spread server3.\n");

	//connect to spread.
	sp_time test_timeout;
	test_timeout.sec = 5;
	test_timeout.usec = 0;
	sprintf( Spread_name, "9625");
	ret = SP_connect_timeout( Spread_name, server_list[server_id], 0, 1, &Mbox, Private_group, test_timeout );
	if( ret != ACCEPT_SESSION )
	{
		SP_error( ret );
		Bye();
	}
	printf("connected to spread server.\n");
	set_mbox(Mbox);

	//Join required mailboxes.
	SP_join ( Mbox, server_list[server_id]);
	SP_join ( Mbox, ALL_SVR);
	SP_join ( Mbox, ALL_TEST);
	SP_join ( Mbox, SVR_PRES);
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
					if (!strcmp(sender, ALL_SVR))//what servers are visible.
					{
reset_svr_upd_received();
						num_servers_present = 0;
						for (int m = 0; m < 5; m++)
						{
							servers_present[i] = 0;
						}
						for( i=0; i < num_groups; i++ )
						{
							if (!strncmp( &target_groups[i][1], BS1, 3))
							{
								printf("server 1 present");
								servers_present[0] = 1;
								num_servers_present++;
							}else if (!strncmp( &target_groups[i][1], BS2, 3))
							{
								printf("server 2 present");
								servers_present[1] = 1;
								num_servers_present++;
							}else if (!strncmp( &target_groups[i][1], BS3, 3))
							{
								printf("server 3 present");
								servers_present[2] = 1;
								num_servers_present++;
							}else if (!strncmp( &target_groups[i][1], BS4, 3))
							{
								printf("server 4 present");
								servers_present[3] = 1;
								num_servers_present++;
							}else if (!strncmp( &target_groups[i][1], BS5, 3))
							{
								printf("server 5 present");
								servers_present[4] = 1;
								num_servers_present++;
							}
						}
						if (num_servers_present>1)
						{
							send_view();
						}
					}					printf("Due to the JOIN of %s\n", memb_info.changed_member );
				}else if( Is_caused_leave_mess( service_type ) ){

					printf("Due to the LEAVE of %s\n", memb_info.changed_member );
				}else if( Is_caused_disconnect_mess( service_type ) ){
					if (!strcmp(sender, ALL_TEST) || !strcmp(sender, SVR_PRES) || !strcmp(sender, server_list[server_id]) || !strcmp(sender, ALL_SVR))//Stay in known groups
					{
						printf("not leaving core groups.\n");
					}else
					{
						if (num_groups == 1)
						{
							printf("Leaving group %s.\n", sender);
							SP_leave ( Mbox, sender);
						}
					}
					printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
				}else if( Is_caused_network_mess( service_type ) ){
					////////////////////////////////////////////////////////////////
					//added to handle spmonitor changes.
					///////////////////////////////////////////////////////////////
if (!strcmp(sender, ALL_SVR))//what servers are visible.
					{
reset_svr_upd_received();
						num_servers_present = 0;
						for (int m = 0; m < 5; m++)
						{
							servers_present[i] = 0;
						}
						for( i=0; i < num_groups; i++ )
						{
							if (!strncmp( &target_groups[i][1], BS1, 3))
							{
								printf("server 1 present");
								servers_present[0] = 1;
								num_servers_present++;
							}else if (!strncmp( &target_groups[i][1], BS2, 3))
							{
								printf("server 2 present");
								servers_present[1] = 1;
								num_servers_present++;
							}else if (!strncmp( &target_groups[i][1], BS3, 3))
							{
								printf("server 3 present");
								servers_present[2] = 1;
								num_servers_present++;
							}else if (!strncmp( &target_groups[i][1], BS4, 3))
							{
								printf("server 4 present");
								servers_present[3] = 1;
								num_servers_present++;
							}else if (!strncmp( &target_groups[i][1], BS5, 3))
							{
								printf("server 5 present");
								servers_present[4] = 1;
								num_servers_present++;
							}
						}
						if (num_servers_present>1)
						{
							send_view();
						}
					}
/////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
					if (!strcmp(sender, ALL_TEST) || !strcmp(sender, SVR_PRES) || !strcmp(sender, server_list[server_id]) || !strcmp(sender, ALL_SVR))//Stay in known groups
					{
						printf("not leaving core groups.\n");
					}else
					{
						if (num_groups == 1)
						{
							printf("Leaving group %s.\n", sender);
							SP_leave ( Mbox, sender);
						}
					}
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

	Bye();
	return( 0 );
}

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
			printf("leaving group %s.\n", join_grp);
			break;
		case REQ_SEND://received email from user.
			add_message(msg+sizeof(int));
			u_email_w(msg+sizeof(int));
			add_update_list(UPD_MSG, msg+sizeof(int));
			print_msgs();
			//printf("this is the subject %s.\n", head_user->head_email->subject);
			break;
		case REQ_HEAD:
			send_header(Mbox, msg+sizeof(int));
			break;
		case REQ_DEL:
			//implement del message.
			del_message(msg+sizeof(int));
			send_header(Mbox, msg+(sizeof(int)*3));
			u_del_w(msg+sizeof(int));
			add_update_list(UPD_DEL, msg+sizeof(int));
			break;
		case REQ_MSG:
			if (!req_message(Mbox, msg+sizeof(int)))
			{
				u_read_w(msg+sizeof(int));
				add_update_list(UPD_READ, msg+sizeof(int));
			}
			break;
		case VIEW:
			merge_view(Mbox, msg+sizeof(int), servers_present, num_servers_present);
			break;
		case UPD_MSG:
			periodic_mat();
			if(check_update(msg))
			{
				printf("received update of type message.");
				update_message(msg);
				write_update(msg);
				list_update(msg);
			}
			break;
		case UPD_DEL:
			periodic_mat();
			if (check_update(msg))
			{
				printf("received update of type delete.");
				update_delete(msg);
				write_update(msg);
				list_update(msg);
			}
			break;
		case UPD_READ:
			periodic_mat();
			if(check_update(msg))
			{
				printf("received update of type read.");
				update_read(msg);
				write_update(msg);
				list_update(msg);
			}
			break;
		default:
			printf("\nUnknown command received45. type- %d\n", msg_type);
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
