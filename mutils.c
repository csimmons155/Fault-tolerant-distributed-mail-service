////////////////////////////////////////////////////////////////////////////////
//
//    James D. Browne
//    Eric
//    Distributed Assignment 2
//
//    mutils.c
//    Functions required by mcast.c
//    Date: 2016/10/10
//
////////////////////////////////////////////////////////////////////////////////
#include "mutils.h"

void get_user_box(user_list* user , char* to_user, user_list* head);

////////////////////////////////////////////////////////////////////////////////
//add received message to the mailbox.
///////////////////////////////////////////////////////////////////////////////
void add_message(int msg_id, int rec_svr, char* buffer, user_list* head)
{
	int total_length = 0;
	email_list* email = malloc(sizeof(email_list));
	//get all of the required strings.
	strncpy(email->to_name, buffer+total_length, LEN_USER);
	total_length += LEN_USER;
	strncpy(email->from_name, buffer+total_length, LEN_USER);
	total_length += LEN_USER;
	strncpy(email->subject, buffer+total_length, LEN_SUB);
	total_length += LEN_SUB;
	strncpy(email->msg, buffer+total_length, LEN_MSG);
	email->read = 0;
	email->rec_svr = rec_svr;
	email->msg_id = msg_id;
	//printf("got a message:\n\nto: %s\nfrom: %s\nsubject: %s\nmsg: %s\n", to_user, from_user, subject, email);

	user_list* user = NULL;
	get_user_box(user, email->to_name, head);
	printf("made it here");
	fflush(stdout);

	if (user->tail_email == NULL)
	{
		user->tail_email = email;
	}else
	{
		user->tail_email->next_email = email;
		user->tail_email = email;
	}
}

///////////////////////////////////////////////////////////////////////////////
//search for to_user's mailbox.  if doesn't exist make it.  mailbox
//address.
/////////////////////////////////////////////////////////////////////////////
void get_user_box(user_list* user , char* to_user, user_list* head)
{
	user_list* temp = head;
	while (temp != NULL)
	{
		if (!strcmp(temp->user_name, to_user))
		{
			user = temp;
			return;
		}else
		{
			temp = temp->next_user;
		}
	}
	printf("made it here");
	fflush(stdout);

	user = malloc(sizeof(user_list));
	strcpy(user->user_name,to_user);
	user->head_email = NULL;
	user->tail_email = NULL;
	user->next_user = head;
	head = user;
}

//////////////////////////////////////////////////////////////////////////////
//Search for message in user's box.  If it exists remove it.
//If it doesn't exist then continue.
/////////////////////////////////////////////////////////////////////////////
void del_message(int msg_id, int rec_svr, char* user_name, user_list* head)
{
	user_list* temp = head;
	while (temp != NULL)
	{
		if (!strcmp(temp->user_name, user_name))
		{
			break;
		}else
		{
			temp = temp->next_user;
		}
	}
	if (temp != NULL)
	{
		printf("user's box was not found\n");
		return;
	}

	email_list* temp_email = temp->head_email;
	email_list* prev_email = NULL;
	while(temp_email != NULL)
	{
		if (temp_email->msg_id == msg_id)
		{
			if (temp_email->rec_svr == rec_svr)
			{
				if(prev_email == NULL)
				{
					temp->head_email = temp_email->next_email;
					free(temp_email);
				}else
				{

				}
			}
		}
	}
}
/*
///////////////////////////////////////////////////////////////////////
//receive start message.  No timeout, reliable receive, print wait status.
//When start message received, end function
void rx_start(int sr)
{
fd_set             mask;
fd_set             dummy_mask;
int                bytes;
int                num;
char               mess_buf[MAX_PACKET_LEN];
char			   targetStr[6] = "start";
targetStr[5] =0;
struct timeval timeout;

FD_ZERO( &mask );
FD_ZERO( &dummy_mask );
FD_SET( sr, &mask );
for(;;)
{
fd_set temp_mask = mask;
timeout.tv_sec = 10;
timeout.tv_usec = 0;

num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
if (num > 0)
{
if ( FD_ISSET( sr, &temp_mask) )
{
bytes = recv( sr, mess_buf, sizeof(mess_buf), 0 );
mess_buf[bytes] = 0;
if (!strncmp(targetStr, mess_buf, bytes))
{
printf("\nstarting process\n");
return;
}
}
} else
{
printf(".");
fflush(stdout);
}
}
}

/////////////////////////////////////////////////////////////////////////////////////////
// receive and process packets until token is received.
// /////////////////////////////////////////////////////////////////////////////////////
void rx_probe(int sr, int ss, int addr, ran_block *start, int num_blocks_used, int *token, int *token_old, int loss_rate)
{
int num_to_jump = 0;
int buff[BUFF_SIZE];
int stoptimer = 0;
while (1)
{
int bytes = rx(buff, sr, loss_rate);
if (bytes>0)
{
if (buff[PACKET_TYPE] == TYPE_TOKEN)
{
if (buff[TOKEN_ID] > token_old[TOKEN_ID])
{
memcpy(token, buff, bytes);
break;
}
}
else if(buff[PACKET_TYPE] == TYPE_DATA)
{
int temp = (num_blocks_used+1) * BLOCK_SIZE;
ran_block *temp_block= start;
while (temp <= buff[PACK_ID])
{
	if (temp_block->next == NULL)
	{
		temp_block->next = malloc (sizeof(ran_block));
		temp_block->next->num_used = 0;
		temp_block->next->next = NULL;
		memset(temp_block->next->block, 0 , BLOCK_SIZE*sizeof(wd));
	}
	temp_block = temp_block->next;
	temp += BLOCK_SIZE;
}
temp_block->block[buff[PACK_ID]%BLOCK_SIZE].random_data= buff[RAN_NUM];
temp_block->block[buff[PACK_ID]%BLOCK_SIZE].machine_index= buff[M_INDEX];
temp_block->block[buff[PACK_ID]%BLOCK_SIZE].packet_index= buff[PACK_ID];
}
}else if (bytes < 0)
{
	stoptimer++;
	if (stoptimer > TOKEN_RESEND_COUNT)
	{
		return;
	}
	tx(token, ((H_SIZE+token[CONT])*sizeof(int)), addr, ss);
}
}
}

//////////////////////////////////////////////////////////////////////////////////////
// send a buffer of info to specified address.
// ///////////////////////////////////////////////////////////////////////////////////
void tx(int *buff,int buff_len, int addr, int ss)
{
	struct sockaddr_in send_addr;
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr.s_addr = addr; // this is problematic because of the htonsl
	send_addr.sin_port = htons (PORT);
	sendto(ss, buff, buff_len, 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
}

////////////////////////////////////////////////////////////////////////////////////
// receive a packet of info and pass to caller.
// ///////////////////////////////////////////////////////////////////////////////
int rx(int *buff, int sr, int loss_rate)
{
	fd_set mask, temp_mask, dummy_mask;
	FD_ZERO (&mask);
	FD_ZERO (&dummy_mask);
	FD_SET (sr, &mask);
	struct timeval timeout;
	struct sockaddr_in from_addr;
	timeout.tv_sec = TIMEOUT_SEC;
	timeout.tv_usec = TIMEOUT_USEC;
	temp_mask = mask;
	int num = select (FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
	if (num > 0)
	{
		if (FD_ISSET (sr, &temp_mask))
		{
			socklen_t from_len = sizeof (from_addr);
			int bytes = recv_dbg (sr, (char *)buff, BUFF_SIZE*sizeof(int), 0);
			return bytes;
		}
	}
	else
	{
		return -1;
	}
}

////////////////////////////////////////////////////////////////////////////////////
// Configure a socket to send MC traffic.
//
int initSM()
{
	int ss;
	int ttl_val;

	ss = socket(AF_INET, SOCK_DGRAM, 0);
	if (ss<0) {
		perror("Mcast: socket");
		exit(1);
	}

	ttl_val = 1;
	if (setsockopt(ss, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val,
				sizeof(ttl_val)) < 0)
	{
		printf("Mcast: problem in setsockopt of multicast ttl %d - ignore in WinNT or Win95\n", ttl_val );
	}

	return ss;
}

///////////////////////////////////////////////////////////////////////////////////////
//configure a socket to receive multicast traffic.
int initRM(int mcast_addr)
{
	int sr;
	struct ip_mreq mreq;
	struct sockaddr_in mrecv_addr;

	sr = socket(AF_INET, SOCK_DGRAM, 0); // socket for receiving
	if (sr<0) {
		perror("Mcast: socket");
		exit(1);
	}

	mrecv_addr.sin_family = AF_INET;
	mrecv_addr.sin_addr.s_addr = INADDR_ANY;
	mrecv_addr.sin_port = htons(PORT);

	if ( bind( sr, (struct sockaddr *)&mrecv_addr, sizeof(mrecv_addr) ) < 0 ) {
		perror("Mcast: bind");
		exit(1);
	}

	mreq.imr_multiaddr.s_addr = mcast_addr;

	// the interface could be changed to a specific interface if needed
	mreq.imr_interface.s_addr = htonl( INADDR_ANY );

	if (setsockopt(sr, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq,
				sizeof(mreq)) < 0)
	{
		perror("Mcast: problem in setsockopt to join multicast address" );
	}
	return sr;
}

////////////////////////////////////////////////////////////////////////////
//configure a socket to use unicast traffic.
int initSU()
{
	int su = socket (AF_INET, SOCK_DGRAM, 0);
	if (su < 0)
	{
		perror ("Ucast: socket");
		exit(1);
	}
	return su;
}

//////////////////////////////////////////////////////////////////////////////////////
//send window of random numbers.
//////////////////////////////////////////////////////////////////////////////////////
void send_window(int window_size, int *token, ran_block *start_ptr, int blocks_used, int midx, int addr, int sms)
{
	int ran_num;
	int block_start = blocks_used * BLOCK_SIZE;
	int block_stop = block_start + BLOCK_SIZE;
	ran_block *temp_block_ptr = start_ptr;
	int buff[PACK_SIZE];

	for (int i = 0; i < window_size; i++)
	{
		token[SEQ]++;//this is the packet id we are going to send.
		do
		{
			ran_num = rand();
		}while(!ran_num);// make sure ran_num != 0

		//save the ran num in our ran_block.
		if (token[SEQ] < block_start)
		{
			printf("trying to write ran number below threshold.\n");
		}
		while (token[SEQ] >= block_stop)
		{
			if (temp_block_ptr->next == NULL)
			{

				//malloc another block.
				temp_block_ptr->next = malloc (sizeof(ran_block));
				temp_block_ptr->next->num_used = 0;
				temp_block_ptr->next->next = NULL;
				memset(temp_block_ptr->next->block, 0 , BLOCK_SIZE*sizeof(wd));
			}
			temp_block_ptr = temp_block_ptr->next;
			block_stop+=BLOCK_SIZE;
		}
		temp_block_ptr->block[token[SEQ]%BLOCK_SIZE].random_data= ran_num;
		temp_block_ptr->block[token[SEQ]%BLOCK_SIZE].machine_index= midx;
		temp_block_ptr->block[token[SEQ]%BLOCK_SIZE].packet_index= token[SEQ];

		//send packet.
		buff[PACKET_TYPE] = TYPE_DATA;
		buff[PACK_ID] = token[SEQ];
		buff[RAN_NUM] = ran_num;
		buff[M_INDEX] = midx;
		tx(buff, PACK_SIZE*sizeof(int), addr , sms);
		fflush(stdout);
	}
}

////////////////////////////////////////////////////////////////////////////
//This is used to process the CLI.
//its not very robust, will break if wrong input allowed.
////////////////////////////////////////////////////////////////////////////
int get_cli(long *num_packets, int *machine_index, int *num_machines, int *loss_rate, char *argv[], int argc)
{
	int success = 0;
	if (argc < 5)
	{
		perror
			("mcast <num of packets> <machine index> <number of machines> <loss rate>\n\
			 where:\n\
			 <num of packets> is the number of packets to send\n\
			 <machine index> is (0-9)\n\
			 <number of machines> is (1-10)\n\
			 <loss rate> is the percent, 0-20, of recieve loss");
		exit (1);
	}

	TRY
	{
		//convert input to values.
		*num_packets = strtol(argv[1], NULL, 0);
		*machine_index = (int) strtol (argv[2], NULL, 0);
		*num_machines = (int) strtol (argv[3], NULL, 0);
		*loss_rate = (int) strtol (argv[4], NULL, 0);
		success = 1;
	}
	CATCH
	{
		success = 0;
	}
	ETRY;

	return success;
}

///////////////////////////////////////////////////////////////////////////
//open a file, fw, for writing.  name the file id#.out
/////////////////////////////////////////////////////////////////////////
FILE* initFile(int procIDX)
{
	char filename[6];
	static FILE *fw;
	snprintf(filename, 6, "%d", procIDX);
	filename[1] = '.';
	filename[2] = 'o';
	filename[3] = 'u';
	filename[4] = 't';
	filename[5] = 0;
	if ((fw = fopen (filename, "w+")) == NULL)
	{
		perror ("fopen");
		exit (0);
	}
	return fw;
}

///////////////////////////////////////////////////////////////////////////////////
// send retrans based on list provided.
// ///////////////////////////////////////////////////////////////////////////
void sendRetrans(int *buff, ran_block *start_ptr, int num_blocks, int machine_index, int sms)
{
	int block_start = num_blocks * BLOCK_SIZE;
	int block_stop = block_start + BLOCK_SIZE;
	ran_block *temp_block_ptr = start_ptr;
	int retrans_buff[PACK_SIZE];

	for (int m = 0; m < buff[CONT]; m++)
	{
		if (buff[m+H_SIZE] < block_start)
		{
			printf("received retrans request for discarded ran_num\n");
		}
		while (buff[m+H_SIZE] >= block_stop)
		{
			if (temp_block_ptr->next == NULL)
			{
				return;
			} else
			{
				temp_block_ptr = temp_block_ptr->next;
				block_stop+=BLOCK_SIZE;
			}
		}
		if (temp_block_ptr->block[buff[m+H_SIZE]%BLOCK_SIZE].random_data)
		{
			retrans_buff[PACKET_TYPE] = TYPE_DATA;
			retrans_buff[PACK_ID] = buff[m+H_SIZE];
			retrans_buff[RAN_NUM] = temp_block_ptr->block[buff[m+H_SIZE]%BLOCK_SIZE].random_data;
			retrans_buff[M_INDEX] = temp_block_ptr->block[buff[m+H_SIZE]%BLOCK_SIZE].machine_index;
			tx(retrans_buff, PACK_SIZE*sizeof(int), htonl(MCAST_ADDR), sms);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
//This will get all the IPs and store them in the IP_list.  This function
//assumes we can use lossless receive for initialization.
void getIPs(int *IP_list, int sr, int ss, int mcast_addr, int num_machines, int machine_index)
{
	struct sockaddr_in from_addr;
	struct sockaddr_in send_addr;
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr.s_addr = mcast_addr;
	send_addr.sin_port = htons (PORT);

	fd_set             mask;
	fd_set             dummy_mask;
	int                bytes;
	int                num;
	int               mess_buf[5];

	int recd = 0; // how many IPs do I already have.

	FD_ZERO( &mask );
	FD_ZERO( &dummy_mask );
	FD_SET( sr, &mask );

	for (int i = 0; i < num_machines; i++)//zero the IP list.
	{
		IP_list[i]=0;
	}
	//send a message with 4 zeros followed by your process ID.
	mess_buf[0]=0;
	mess_buf[1]=0;
	mess_buf[2]=0;
	mess_buf[3]=0;
	mess_buf[4]=machine_index;

	sendto(ss, mess_buf, 5*sizeof(int), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
	printf("starting to collect IPs\n");
	for(;;)
	{
		fd_set temp_mask = mask;

		num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, 0);
		if (num > 0)
		{
			if ( FD_ISSET( sr, &temp_mask) )
			{
				socklen_t from_len = sizeof (from_addr);
				bytes = recvfrom( sr, mess_buf, sizeof(mess_buf), 0,
						(struct sockaddr *) &from_addr, &from_len);
				if(bytes == 5 * sizeof(int))
				{
					int fromIDX = mess_buf[4];
					if (!IP_list[fromIDX])
					{
						IP_list[fromIDX]= from_addr.sin_addr.s_addr;
						recd++;
						if (recd == num_machines){
							return;
						}
					}
				}
			}
		}
	}
	printf("done collecting IPs\n");
}

////////////////////////////////////////////////////////////////////////////////
//Set initial values for current and old token.
///////////////////////////////////////////////////////////////////////////////
void init_tokens(int *token, int *token_old)
{
	token[PACKET_TYPE] = TYPE_TOKEN;
	token[TOKEN_ID] = -1;
	token[SEQ] = -1;
	token[ARU] = -1;
	token[ARU_ID] = 11;
	token[CONT] = 0;

	token_old[PACKET_TYPE] = TYPE_TOKEN;
	token_old[TOKEN_ID] = -1;
	token_old[SEQ] = -1;
	token_old[ARU] = -2;
	token_old[ARU_ID] = 11;
	token_old[CONT] = 0;

	//memcpy(token_old, token, H_SIZE * sizeof(int));
}

////////////////////////////////////////////////////////////////////////////////
//send token calculates aru, adds required nacks, then sends token unicast to
//next id. aru = last successfully received in order packet.
///////////////////////////////////////////////////////////////////////////////
void send_token(int *token, ran_block *start, int num_blocks_used, int midx, int num_macs, int *ips, int ss, int *my_aru)
{
	int start_block = num_blocks_used * BLOCK_SIZE;
	int stop_block = start_block + BLOCK_SIZE;
	int index = *my_aru+1;
	int last_look = BLOCK_SIZE;
	int my_aru_flag = 0;
	int addr;
	ran_block *temp = start;

	token[CONT] = 0;

	while (index >= stop_block)
	{
		temp = temp->next;
		if (temp == NULL)
		{
			break;
		}
		stop_block += BLOCK_SIZE;
		start_block += BLOCK_SIZE;
	}

	while (temp != NULL)
	{
		if (temp->next == NULL){
			last_look = token[SEQ]%BLOCK_SIZE + 1;
		}
		for (int m = index%BLOCK_SIZE; m < last_look; m++)
		{
			if (!temp->block[m].random_data)
			{
				token[token[CONT] + H_SIZE] = m + start_block;
				token[CONT]++;
				if (!my_aru_flag)
				{
					my_aru_flag = 1;
					*my_aru = m+start_block-1;
				}
				if (token[CONT] == (BUFF_SIZE - H_SIZE))
				{
					break;
				}
			}
		}
		temp = temp->next;
		index = 0;
		stop_block += BLOCK_SIZE;
		start_block += BLOCK_SIZE;

		if (token[CONT] == (BUFF_SIZE - H_SIZE))
		{
			break;
		}
	}

	if (!my_aru_flag)
	{
		*my_aru = token[SEQ];
	}

	token[TOKEN_ID]++;

	addr = ips[(midx+1)%num_macs];

	if (token[ARU_ID] == 11)
	{
		token[ARU] = *my_aru;
		token[ARU_ID] = midx;
	}

	if (*my_aru < token[ARU])
	{
		token[ARU] = *my_aru;
		token[ARU_ID] = midx;
	}else if (token[ARU_ID] == midx)
	{
		token[ARU] = *my_aru;
	}

	if (token[SEQ] == token[ARU])
	{
		token[ARU_ID] = 11;
	}
	for (int n = 0 ; n < token[CONT]; n++){
	}
	tx(token, ((H_SIZE+token[CONT])*sizeof(int)), addr, ss);
}

//////////////////////////////////////////////////////////////////////////////
// if any blocks exist below global_aru then write them to disk and free block.
// ///////////////////////////////////////////////////////////////////////////
void clean_block(ran_block **start_ptr, int *num_blocks_used, int global_aru, FILE *fw)
{
	int stop_point = (*num_blocks_used+1) * BLOCK_SIZE;
	ran_block *temp = *start_ptr;
	while(global_aru >= stop_point)
	{
		for (int q = 0; q < BLOCK_SIZE; q++)
		{
			fprintf(fw, "%2d, %8d, %8d\n", (*start_ptr)->block[q].machine_index, (*start_ptr)->block[q].packet_index, (*start_ptr)->block[q].random_data);
		}
		(*num_blocks_used)++;
		if((*start_ptr)->next == NULL)
		{
			fflush(0);
			(*start_ptr)->next = malloc(sizeof(ran_block));
			(*start_ptr)->next->num_used = 0;
			(*start_ptr)->next->next = NULL;
			memset((*start_ptr)->next->block, 0 , BLOCK_SIZE*sizeof(wd));
		}

		*start_ptr = (*start_ptr)->next;

		free(temp);
		//just added this.  May not work.
		temp = *start_ptr;
		stop_point+=BLOCK_SIZE;
	}
}

//////////////////////////////////////////////////////////////////////////////////
// empty out the last block of random numbers prior to closing the file pointer.
// ///////////////////////////////////////////////////////////////////////////////
void clean_last_block(ran_block **start_ptr, FILE *fw)
{
	int last_used;
	ran_block *temp = *start_ptr;
	while(*start_ptr != NULL)
	{
		for (int m = 0; m < BLOCK_SIZE; m++)
		{
			if (!(*start_ptr)->block[m].random_data)
			{
				last_used = m;
				break;
			}
			if (m == BLOCK_SIZE-1)
			{
				last_used = BLOCK_SIZE;
			}
		}
		for (int q = 0; q < last_used; q++)
		{
			fprintf(fw, "%2d, %8d, %8d\n", (*start_ptr)->block[q].machine_index, (*start_ptr)->block[q].packet_index, (*start_ptr)->block[q].random_data);
		}
		*start_ptr = (*start_ptr)->next;

		free(temp);
		temp = *start_ptr;
	}
}*/
////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//  10/11/2016 EC: edited the rx and tx functions.
//                 - Added a TRY-CATCH implementation to help CLI parse
//
//
/////////////////////////////////////////////////////////////////////////////////
