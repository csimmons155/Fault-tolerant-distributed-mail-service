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
static	char	User[80];
static  char    Spread_name[80];

static  char    Private_group[MAX_GROUP_NAME];
static  mailbox Mbox;
int 			num_in_group = 0;
int 			total_rec = 0;
static  int     To_exit = 0;
static 	FILE 	*fw;

#define MAX_MESSLEN     102400
//#define MAX_VSSETS      10
#define MAX_MEMBERS     100
#define MAX_MSG_SEND	10
#define MSG_LEN			304
#define P_IDX			0
#define M_IDX			1
#define R_NUM			2

int start_proc();
void send_mess(int mess_ind, int proc_ind, int num_to_send);
void rec_mess(int num_to_rec);
void Print_help();
void Bye();
void initFile(int procIDX);

//////////////////////////////////////////////////////////////////////////////
//MAIN
//////////////////////////////////////////////////////////////////////////////
int main( int argc, char *argv[] )
{
	int	    ret;
	int		server_id;
	struct timeval start_time;
	struct timeval end;
	//get command line arguments.
	if (argc == 2)
	{
		server_id = (int) strtol (argv[1], NULL, 0);
	}else
	{
		Print_help();
	}
	//open file
	initFile(server_id);
	//set connection variables.
	sp_time test_timeout;
	test_timeout.sec = 5;
	test_timeout.usec = 0;
	User[0] = 0;
	sprintf( Spread_name, "4803");

	ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group, test_timeout );
	if( ret != ACCEPT_SESSION )
	{
		SP_error( ret );
		Bye();
	}

	ret = SP_join ( Mbox, "Browne");

	printf("joined group.  now waiting on all processes before start\n");

	//Wait for processes before starting.
	while(num_in_group != num_of_processes)
	{
		num_in_group = start_proc();
	}
	//Start process
	gettimeofday(&start_time, NULL);
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
	gettimeofday(&end, NULL);

	float time_elapsed_sec = end.tv_sec - start_time.tv_sec;
	float time_elapsed_usec = end.tv_usec - start_time.tv_usec;
	printf("Time elapsed: %f\n", time_elapsed_sec + time_elapsed_usec/1000000);
	printf("Total packets received: %i\n", total_rec);
	printf("Speed: %fMbit/s\n", ((float)total_rec*1200.0*8.0 / 1000000.0)/(time_elapsed_sec + time_elapsed_usec / 1000000));

	Bye();
	return( 0 );
}

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

//////////////////////////////////////////////////////////////////////////
//Print the usage message.
/////////////////////////////////////////////////////////////////////////
void    Print_help()
{
	printf( "Usage: mcast <number of messages> <process index> <num of processors>\n");
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
////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//  11/15/2016 JB: Cleaned up code from project 3 to use in project 4.
//
//
/////////////////////////////////////////////////////////////////////////////////
