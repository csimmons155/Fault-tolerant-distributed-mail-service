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

user_list 		*head_user = NULL;
update_list 	*head_update = NULL;
update_list 	*tail_update = NULL;
static 	FILE 	*fw;
int 			svr_id;
mailbox 		m_box;
int				mid = 0;
int 			svr_know[5][5] = { 0 };
int 			svr_upd_received[5];//keep track of how many updates have been received from all servers in the group.
int 			min_updates[5] = { 0 };
int 			max_updates[5] = {0};

static  void	Bye(mailbox Mbox);
void delete_user(char* user_name);
void print_msgs();
void send_updates(int upd_id, int svr_id);
void send_update(int upd_id, int svr_id);
void print_view();
void print_list();

void reset_svr_upd_received()
{
	for(int i = 0; i < 5; i++)
	{
	svr_upd_received[i] = 0;
min_updates[i] = svr_know[svr_id][i];
			max_updates[i] = svr_know[svr_id][i];
	}
}
////////////////////////////////////////////////////////////////
//print the contents of the update list.
///////////////////////////////////////////////////////////////
void print_list()
{
update_list *temp = head_update;
printf("\n\n\nprinting list:\n");
while (temp!= NULL)
{
printf("type %d, svr %d, id %d\n",temp->update_type, temp->rec_svr, temp->msg_id);
temp = temp->next_update;
}
}
////////////////////////////////////////////////////////////////
//Print everyone's messages.
///////////////////////////////////////////////////////////////
void print_msgs()
{
	user_list* temp = head_user;
	email_list* temp_email;
	while(temp != NULL)
	{
		printf("user is %s\n", temp->user_name);
		temp_email = temp->head_email;
		while(temp_email != NULL)
		{
			printf("subject is %s\t%d\t%d.\n", temp_email->subject, temp_email->rec_svr, temp_email->msg_id);
			temp_email = temp_email->next_email;
		}
		temp = temp->next_user;
	}
}

/////////////////////////////////////////////////////////////////////////
//set the svr_id, for use in writing and reading from file.
/////////////////////////////////////////////////////////////////////////
void set_proc(int proc_id)
{
	svr_id= proc_id;
}

/////////////////////////////////////////////////////////////////////////
//set the mailbox for use in procs.
/////////////////////////////////////////////////////////////////////////
void set_mbox(mailbox Mbox)
{
	m_box = Mbox;
}

///////////////////////////////////////////////////////////////////////////////
//Print the view to screen.
///////////////////////////////////////////////////////////////////////////////
void print_view()
{
	printf("\t   1   2   3   4   5");
	for (int m = 0; m < 5; m++)
	{
		printf("\n\t%d  ", m+1);
		for(int n = 0; n < 5; n++)
		{
			printf("%d   ", svr_know[m][n]);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
//add received message from user
///////////////////////////////////////////////////////////////////////////////
void add_message( char* buffer)
{
	int total_length = 0;
	mid++;
	email_list* email = malloc(sizeof(email_list));
	//get all of the required strings.
	strncpy(email->to_name, buffer+total_length, LEN_USER);
	total_length += LEN_USER;
	strncpy(email->from_name, buffer+total_length, LEN_USER);
	total_length += LEN_USER;
	strncpy(email->subject, buffer+total_length, LEN_SUB);
	total_length += LEN_SUB;
	strncpy(email->msg, buffer+total_length, LEN_MSG);
	email->next_email = NULL;
	email->read = 0;
	email->rec_svr = svr_id;
	email->msg_id = mid;

	user_list* temp = head_user;
	while (temp != NULL)
	{
		if (!strcmp(temp->user_name, email->to_name))
		{
			break;
		}else
		{
			temp = temp->next_user;
		}
	}

	if(temp == NULL)
	{
		temp = malloc(sizeof(user_list));
		strncpy(temp->user_name,email->to_name, LEN_USER);
		temp->head_email = NULL;
		temp->tail_email = NULL;
		temp->next_user = head_user;
		head_user = temp;
	}
	if (temp->tail_email == NULL)
	{
		temp->tail_email = email;
		temp->head_email = email;
	}else
	{
		temp->tail_email->next_email = email;
		temp->tail_email = email;
	}
}

////////////////////////////////////////////////////////////////////////////
//Send header to user
///////////////////////////////////////////////////////////////////////////
void send_header(mailbox Mbox, char* buffer)
{
	char username[LEN_USER];
	char group_name[MAX_GROUP_NAME];
	int total_length = 0;
	int msg_type;
	int transfer_to_msg;
	char msg[MAX_MSG];
	int tot_sub = 0;
	int sub_for_pack;
	email_list* e_list;
	int tot_msg = 0;
	int ret;
	//get all of the required strings.
	strncpy(username, buffer+total_length, LEN_USER);
	total_length += LEN_USER;
	strncpy(group_name, buffer+total_length, MAX_GROUP_NAME);
	printf("sending %s's list to %s group.\n", username, group_name);
	print_msgs();

	user_list* temp = head_user;
	while(temp != NULL)
	{
		if (!strncmp(temp->user_name, username, LEN_USER))
		{
			break;
		}else
		{
			temp = temp->next_user;
		}
	}

	if (temp == NULL)
	{
		printf("user %s not found.\n", username);

		msg_type = SEND_NOHD;
		memcpy(msg, &msg_type, sizeof(int));

		ret= SP_multicast( Mbox, SAFE_MESS, group_name, 1, sizeof(int), msg );
		if( ret < 0 )
		{
			SP_error( ret );
			Bye(Mbox);
		}
	}else
	{
		e_list = temp->head_email;
		while(e_list != NULL)
		{
			tot_sub++;
			e_list = e_list->next_email;
		}
		if (tot_sub%SUB_PER == 0)
		{
			tot_msg = tot_sub/SUB_PER;
		}else
		{
			tot_msg = tot_sub/SUB_PER + 1;
		}
		msg_type = SEND_HEAD;
		memcpy(msg, &msg_type, sizeof(int));//add message type
		memcpy(msg+sizeof(int), &tot_msg, sizeof(int));//add how many packs required.
		e_list = temp->head_email;

		for (int q = 0; q < tot_msg; q++)
		{
			total_length = sizeof(int) + sizeof(int);
			memcpy(msg+total_length, &q, sizeof(int));//add which packet this is.
			total_length+= sizeof(int);
			if (tot_sub >= SUB_PER)
			{
				sub_for_pack = SUB_PER;
				tot_sub = tot_sub - SUB_PER;
			}else
			{
				sub_for_pack = tot_sub;
			}
			memcpy(msg+total_length, &sub_for_pack, sizeof(int));
			total_length += sizeof(int);
			for (int m = 0; m < sub_for_pack; m++)
			{
				transfer_to_msg = e_list->read;
				memcpy(msg+total_length,&transfer_to_msg, sizeof(int));
				total_length += sizeof(int);
				transfer_to_msg = e_list->rec_svr;
				memcpy(msg+total_length,&transfer_to_msg, sizeof(int));
				total_length += sizeof(int);
				transfer_to_msg = e_list->msg_id;
				memcpy(msg+total_length,&transfer_to_msg, sizeof(int));
				total_length += sizeof(int);
				strncpy(msg+total_length, e_list->from_name, LEN_USER);
				total_length += LEN_USER;
				strncpy(msg+total_length, e_list->subject, LEN_SUB);
				total_length += LEN_SUB;

				e_list = e_list->next_email;
			}
			ret= SP_multicast( Mbox, SAFE_MESS, group_name, 1, total_length, msg );
			if( ret < 0 )
			{
				SP_error( ret );
				Bye(Mbox);
			}
		}
	}
}

//////////////////////////////////////////////////////////////
//an error happened that the process won't recover from.
////////////////////////////////////////////////////////////////
static  void	Bye(mailbox Mbox)
{

	printf("\nBye.\n");

	SP_disconnect( Mbox );

	exit( 0 );
}

//////////////////////////////////////////////////////////////////////////////
//Search for message in user's box.  If it exists remove it.
//If it doesn't exist then continue.
/////////////////////////////////////////////////////////////////////////////
void del_message(char* msg)
{
	int msg_id;
	int rec_svr;

	char user_name[LEN_USER];
	rec_svr = ((int*)msg)[0];

	msg_id = ((int*)msg)[1];
	strncpy(user_name, msg+sizeof(int)*2, LEN_USER);

	//	printf("looking for %s.\n", user_name);
	user_list* temp = head_user;
	while (temp != NULL)
	{
		if (!strcmp(temp->user_name, user_name))
		{
			printf("found box for %s\n", user_name);
			break;
		}else
		{
			temp = temp->next_user;
		}
	}
	if (temp == NULL)
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
					if (temp->head_email == NULL)
					{
						delete_user(temp->user_name);
					}
				}else
				{
					prev_email->next_email = temp_email->next_email;
					if(temp_email->next_email == NULL)
					{
						temp->tail_email = prev_email;
					}
					free(temp_email);
				}
				printf("message deleted\n");
				return;
			}
		}
		prev_email = temp_email;
		temp_email = temp_email->next_email;
	}
}

//////////////////////////////////////////////////////////////////////////////
//delete a user if they don't have any emails.
////////////////////////////////////////////////////////////////////////////
void delete_user(char* user_name)
{
	user_list* temp = head_user;
	user_list* prev = NULL;
	while (temp != NULL)
	{
		if (!strcmp(temp->user_name, user_name))
		{
			break;
		}else
		{
			prev = temp;
			temp = temp->next_user;
		}
	}
	if(temp==NULL)
	{
		printf("error- user was not found so cannot be deleted.");
		return;
	}
	if(prev == NULL)
	{
		head_user =head_user->next_user;
	}else
	{
		prev->next_user = temp->next_user;
	}
	printf("user was deleted\n");
	free(temp);
	print_msgs();
}

//////////////////////////////////////////////////////////////////////////////
//Find the message and send it to the user.
//If it doesn't exist then inform the user.
/////////////////////////////////////////////////////////////////////////////
int req_message(mailbox Mbox, char* msg)
{
	int msg_id;
	int rec_svr;
	int msg_type;
	char user_name[LEN_USER];
	char group[MAX_GROUP_NAME];
	int runner = 0;
	char msgS[MAX_MSG];
	rec_svr = ((int*)msg)[0];
	msg_id = ((int*)msg)[1];
	runner = sizeof(int)*2;
	strncpy(user_name, msg+runner, LEN_USER);
	runner+= LEN_USER;
	strncpy(group, msg+runner, MAX_GROUP_NAME);

	user_list* temp = head_user;
	while (temp != NULL)
	{
		if (!strcmp(temp->user_name, user_name))
		{
			printf("found box for %s\n", user_name);
			break;
		}else
		{
			temp = temp->next_user;
		}
	}
	if (temp == NULL)
	{
		msg_type = SEND_NOMSG;
		memcpy(msgS, &msg_type, sizeof(int));

		SP_multicast( Mbox, SAFE_MESS, group, 1, sizeof(int), msgS );

		printf("user's box was not found\n");
		return 0;
	}
	email_list* temp_email = temp->head_email;
	while(temp_email != NULL)
	{
		if (temp_email->msg_id == msg_id)
		{
			if (temp_email->rec_svr == rec_svr)
			{
				printf("message found\n");

				msg_type = SEND_MSG;
				runner = 0;
				memcpy(msgS, &msg_type, sizeof(int));//add message type
				runner+=sizeof(int);
				memcpy(msgS+runner, temp_email->from_name, LEN_USER);
				runner+=LEN_USER;
				memcpy(msgS+runner, temp_email->subject, LEN_SUB);
				runner+=LEN_SUB;
				memcpy(msgS+runner, temp_email->msg, LEN_MSG);
				runner+=LEN_MSG;

				SP_multicast( Mbox, SAFE_MESS, group, 1, runner, msgS );

				if(temp_email->read)
				{
					return 1;
				}else
				{
				temp_email->read = 1;
				return 0;
				}
			}
		}
		temp_email = temp_email->next_email;
	}

	msg_type = SEND_NOMSG;
	memcpy(msgS, &msg_type, sizeof(int));

	SP_multicast( Mbox, SAFE_MESS, group, 1, sizeof(int), msgS );

	printf("email was not found.\n");
return 0;
}

////////////////////////////////////////////////////////////////
//write the read update to file.
////////////////////////////////////////////////////////////////
void u_read_w(char* msg)
{
	int msg_id;
	int rec_svr;
	int msg_type;
	int runner = 0;

	char filename[] = "x.db";
	filename[0] = svr_id + '0';
	if ((fw = fopen (filename, "a+")) == NULL)
	{
		perror ("fopen");
		exit (0);
	}
	mid++;
	msg_type = UPD_READ;
	rec_svr = ((int*)msg)[0];
	msg_id = ((int*)msg)[1];
	runner = sizeof(int)*2;
	fwrite((void *)(&msg_type), sizeof(int), 1, fw);
	fwrite((void *)(&svr_id), sizeof(int), 1, fw);//Save the Id of the server that first received this update.
	fwrite((void *)(&mid), sizeof(int), 1, fw);//Save update ID for processing.
	fwrite((void *)(&rec_svr), sizeof(int), 1, fw);
	fwrite((void *)(&msg_id), sizeof(int), 1, fw);
	fwrite((void *)(msg+runner), LEN_USER, 1, fw);
	fclose(fw);
	printf("\nupdate written to disk of type %d\n", msg_type);
}

/////////////////////////////////////////////////////////////////
//write the del update to file.
////////////////////////////////////////////////////////////////
void u_del_w(char* msg)
{
	int msg_type;
	int msg_id;
	int rec_svr;

	char user_name[LEN_USER];

	msg_type = UPD_DEL;
	rec_svr = ((int*)msg)[0];
	msg_id = ((int*)msg)[1];
	strncpy(user_name, msg+sizeof(int)*2, LEN_USER);

	char filename[] = "x.db";
	filename[0] = svr_id + '0';
	if ((fw = fopen (filename, "a+")) == NULL)
	{
		perror ("fopen");
		exit (0);
	}
	mid++;//increment the update counter.
	fwrite((void *)(&msg_type), sizeof(int), 1, fw);
	fwrite((void *)(&svr_id), sizeof(int), 1, fw);//Save the Id of the server that first received this update.
	fwrite((void *)(&mid), sizeof(int), 1, fw);//Save update ID for processing.
	fwrite((void *)(&rec_svr), sizeof(int), 1, fw);
	fwrite((void *)(&msg_id), sizeof(int), 1, fw);
	fwrite((void *)user_name, LEN_USER, 1, fw);
	fclose(fw);
	printf("\nupdate written to disk of type %d\n", msg_type);
}

/////////////////////////////////////////////////////////////////
//write the add update to file.
////////////////////////////////////////////////////////////////
void u_email_w(char* msg)
{
	int msg_type;
	int msg_id;
	int rec_svr;
	int runner;

	msg_type = UPD_MSG;
	rec_svr = svr_id;
	msg_id = mid;

	char filename[] = "x.db";
	filename[0] = svr_id + '0';
	if ((fw = fopen (filename, "a+")) == NULL)
	{
		perror ("fopen");
		exit (0);
	}
	runner = 0;
	fwrite((void *)(&msg_type), sizeof(int), 1, fw);
	fwrite((void *)(&rec_svr), sizeof(int), 1, fw);
	fwrite((void *)(&msg_id), sizeof(int), 1, fw);
	fwrite((void *)(msg+runner), LEN_USER, 1, fw);
	runner+=LEN_USER;
	fwrite((void *)(msg+runner), LEN_USER, 1, fw);
	runner+=LEN_USER;
	fwrite((void *)(msg+runner), LEN_SUB, 1, fw);
	runner+=LEN_SUB;
	fwrite((void *)(msg+runner), LEN_MSG, 1, fw);

	fclose(fw);
}

///////////////////////////////////////////////////////////////////////
//Process file of updates.
/////////////////////////////////////////////////////////////////////
void read_file()
{
	int upd_type;
	int size_msg = sizeof(int)*2+LEN_USER*2+LEN_SUB+LEN_MSG;
	int size_del = sizeof(int)*4+LEN_USER;
	int size_red = sizeof(int)*4+LEN_USER;
	char update[MAX_MSG];
	char filename[] = "x.db";
	filename[0] = svr_id + '0';

	struct stat buffer;

	if(!stat(filename, &buffer))
	{
		if ((fw = fopen (filename, "r")) == NULL)
		{
			perror ("fopen");
			exit (0);
		}
		while(fread((void *)&upd_type, sizeof(int), 1, fw)!= 0)
		{

			if (upd_type == UPD_READ)
			{
				((int *)update)[0] = UPD_READ;
				fread((void *)update+sizeof(int), size_red, 1, fw);
				update_read(update);

			}else if (upd_type == UPD_DEL)
			{
				((int *)update)[0] = UPD_DEL;
				fread((void *)update+sizeof(int), size_del, 1, fw);
				update_delete(update);
			}else if(upd_type == UPD_MSG)
			{
				((int *)update)[0] = UPD_MSG;
				fread((void *)update+sizeof(int), size_msg, 1, fw);
				update_message(update);

			}else
			{
				printf("unrecognized update received: %d\n", upd_type);
				continue;
			}
			list_update(update,upd_type);
print_list();

//print_view();
		}
		printf("trying to read from file..\n");
		fclose(fw);
		printf("\nthe file was processed.\n");
		print_msgs();

	}else
	{
		printf("\nNo database was found\n");
	}
}

////////////////////////////////////////////////////////////////////////////////
//add update: message type, for read from file and updates from server.
///////////////////////////////////////////////////////////////////////////////
void update_message(char* buffer)
{
	int total_length = 0;
	int rec_svr;
	int msg_id;
	rec_svr = ((int*)buffer)[1];
	msg_id = ((int*)buffer)[2];
	//make sure message ID is good.
	if (msg_id > mid)
	{
		mid = msg_id;
	}
	///////////////////////////////

	total_length = sizeof(int)*3;
	email_list* email = malloc(sizeof(email_list));
	//get all of the required strings.
	strncpy(email->to_name, buffer+total_length, LEN_USER);
	total_length += LEN_USER;
	strncpy(email->from_name, buffer+total_length, LEN_USER);
	total_length += LEN_USER;
	strncpy(email->subject, buffer+total_length, LEN_SUB);
	total_length += LEN_SUB;
	strncpy(email->msg, buffer+total_length, LEN_MSG);
	email->next_email = NULL;
	email->read = 0;
	email->rec_svr = rec_svr;
	email->msg_id = msg_id;
	user_list* temp = head_user;
	while (temp != NULL)
	{
		if (!strcmp(temp->user_name, email->to_name))
		{
			break;
		}else
		{
			temp = temp->next_user;
		}
	}
	if(temp == NULL)
	{
		temp = malloc(sizeof(user_list));
		strncpy(temp->user_name,email->to_name, LEN_USER);
		temp->head_email = NULL;
		temp->tail_email = NULL;
		temp->next_user = head_user;
		head_user = temp;
	}
	//find where to insert message.
	if (temp->tail_email == NULL)
	{
		temp->tail_email = email;
		temp->head_email = email;
		return;
	}
	email_list *curr = temp->head_email;
	email_list *prev = NULL;
	while(curr != NULL)
	{
		if (curr->msg_id < email->msg_id)
		{
			prev = curr;
			curr = curr->next_email;
		}else if (curr->msg_id == email->msg_id)
		{
			do{
				if (curr->rec_svr < email->rec_svr && curr->msg_id == email->msg_id)
				{
					prev = curr;
					curr = curr->next_email;
				}else
				{
					break;
				}
			}while (curr != NULL);
			break;
		}else
		{
			break;
		}
	}
	if (prev == NULL)
	{
		email->next_email = temp->head_email;
		temp->head_email = email;
	}else
	{
		prev->next_email = email;
		email->next_email = curr;
		if (curr == NULL)
		{
			temp->tail_email= email;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//add update: delete type, for read from file and updates from other servers.
///////////////////////////////////////////////////////////////////////////////
void update_delete(char* msg)
{
	int rec_svr;
	int msg_id;
	int upd_id = ((int*)msg)[2];
	//make sure message ID is in accordance with everyone.
	if (upd_id > mid)
	{
		mid = upd_id;
	}
	//////////////////////////////////
	char user_name[LEN_USER];

	rec_svr = ((int*)msg)[3];
	msg_id = ((int*)msg)[4];
	strncpy(user_name, msg+sizeof(int)*5, LEN_USER);

	user_list* temp = head_user;
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
	if (temp == NULL)
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
					if (temp->head_email == NULL)
					{
						delete_user(temp->user_name);
					}
				}else
				{
					prev_email->next_email = temp_email->next_email;
					if(temp_email->next_email == NULL)
					{
						temp->tail_email = prev_email;
					}
					free(temp_email);
				}
				return;
			}
		}
		prev_email = temp_email;
		temp_email = temp_email->next_email;
	}
}

//////////////////////////////////////////////////////////////////////////////
//add update: read type, not from user.  Used with file and updates from
//servers.
/////////////////////////////////////////////////////////////////////////////
void update_read(char* msg)
{
	int msg_id;
	int rec_svr;
	int upd_id = ((int*)msg)[2];
	//make sure message ID is good.
	if (upd_id > mid)
	{
		mid = upd_id;
	}
	////////////////////////////////
	char user_name[LEN_USER];
	int runner = 0;
	rec_svr = ((int*)msg)[3];
	msg_id = ((int*)msg)[4];
	runner = sizeof(int)*5;
	strncpy(user_name, msg+runner, LEN_USER);
	user_list* temp = head_user;
	while (temp != NULL)
	{
		if (!strcmp(temp->user_name, user_name))
		{
			printf("found box for %s\n", user_name);
			break;
		}else
		{
			temp = temp->next_user;
		}
	}
	if (temp == NULL)
	{
		printf("user's box was not found\n");
		return;
	}
	email_list* temp_email = temp->head_email;
	while(temp_email != NULL)
	{
		if (temp_email->msg_id == msg_id)
		{
			if (temp_email->rec_svr == rec_svr)
			{
				printf("message found\n");
				temp_email->read = 1;
				return;
			}
		}
		temp_email = temp_email->next_email;
	}
	printf("email was not found.\n");
}

///////////////////////////////////////////////////////////////////////////////
//add update to list for updates received from user
///////////////////////////////////////////////////////////////////////////////
void add_update_list(int update_type, char *buffer)
{
	update_list* update = malloc(sizeof(update_list));
	update->update_type = update_type;
	update->rec_svr = svr_id;
	update->msg_id = mid;
	char msg[MAX_MSG];
	((int *)msg)[0] = update_type;
	((int *)msg)[1] = svr_id;
	((int *)msg)[2] = mid;
	memcpy(msg + (sizeof(int)*3), buffer, MAX_MSG - sizeof(int)*3);

	memcpy(update->buffer, msg, MAX_MSG);
	update->next_update = NULL;
	svr_know[svr_id][svr_id] = mid;
	if (tail_update == NULL)
	{
		tail_update = update;
		head_update = update;
	}else
	{
		tail_update->next_update = update;
		tail_update = update;
	}
print_list();

	SP_multicast( m_box, SAFE_MESS, ALL_SVR, 1, MAX_MSG, update->buffer );
}

/////////////////////////////////////////////////////////////////////////////
//send view sends the matrix view of this process.  format is type, src, matrix
/////////////////////////////////////////////////////////////////////////////
void send_view(mailbox Mbox)
{
	//int msg_type = VIEW ;
	//int ret;
	char msg[sizeof(int)*27];

	((int *)msg)[0] = VIEW;
	((int *)msg)[1] = svr_id;
	memcpy(msg+sizeof(int)*2, svr_know, sizeof(int)*25);

	SP_multicast( Mbox, SAFE_MESS, ALL_SVR , 1, sizeof(msg), msg );
}

///////////////////////////////////////////////////////////////////////////////
//merge received matrices.
///////////////////////////////////////////////////////////////////////////////
void merge_view(mailbox Mbox, char *buffer, int *servers, int num_servers)
{
	//int offset;
	int owner = ((int *)buffer)[0];
//	int matrix[5][5];
//	memcpy(matrix, buffer
	if(num_servers == 1)
	{
		return;
	}

	svr_upd_received[owner]= 1;
	printf("\n\n");
	print_view();
	if (owner != svr_id)
	{
		for(int j = 0; j < 5; j++)
		{

			for(int q = 0; q < 5; q++)
			{
				if(((int *)buffer)[j*5+q+1] > svr_know[j][q])
				{
					svr_know[j][q] = ((int *)buffer)[j*5+q+1];
				}
				if(((int *)buffer)[j*5+q+1] > mid)
				{
					mid = ((int *)buffer)[j*5+q+1];
				}
			}
		}
	}

	for(int s = 0; s < 5; s++)
	{
		if (servers[s])
		{
			for(int m = 0; m < 5; m++)
			{
				if(svr_know[s][m] < min_updates[m])
				{
					min_updates[m] =svr_know[s][m];
				}
				if (svr_know[s][m] > max_updates[m])
				{
					max_updates[m] =svr_know[s][m];
				}
			}
		}
	}
	printf("\n\n");
	print_view();
	if (!memcmp((void *)svr_upd_received, (void *) servers, sizeof(int)*5 ))
	{
for (int w = 0; w < 5; w++)
	{
		printf("the max for %d is %d; the min is %d\n", w, max_updates[w], min_updates[w]);
	}
		printf("\nreceived all updates, starting to send.\n");
		for(int p = 0; p< 5; p++)
		{
			printf("min: %d, max: %d\n", min_updates[p], max_updates[p]);
			if(max_updates[p] == min_updates[p] || max_updates[p] > svr_know[svr_id][p])
			{
				printf("should send 2\n");
				continue;
			}
			for(int u = 0; u < svr_id+1; u++)
			{
				if(servers[u])
				{
					if (u == svr_id)
					{
						printf("should send 1\n");
						send_updates(min_updates[p], p);
					}else if (svr_know[u][p] == max_updates[p])
					{
						break;
					}
				}
			}
		}
	}else
	{
printf("\nreceived mats, need to continue.\n");
	}
}

////////////////////////////////////////////////////////////////////////////////
//send updates to newly merged servers.
///////////////////////////////////////////////////////////////////////////////
void send_updates(int min, int server)
{
	update_list *temp = head_update;
	while (temp!= NULL)
	{
		if(temp->rec_svr == server && temp->msg_id > min)
		{
			SP_multicast( m_box, SAFE_MESS, ALL_SVR, 1, MAX_MSG, temp->buffer );
		}
		temp = temp->next_update;
	}
}

////////////////////////////////////////////////////////////////////////////////
//send the selected update to all servers.
///////////////////////////////////////////////////////////////////////////////
void send_update(int upd_id, int svr_id)
{
	update_list *temp = head_update;
	printf("looking for email\n");
	fflush(stdout);
	while (temp!= NULL)
	{
		if(temp->rec_svr == svr_id && temp->msg_id == upd_id)
		{
			printf("\n\nupdate found and is being sent\n\n");
			SP_multicast( m_box, SAFE_MESS, ALL_SVR, 1, MAX_MSG, temp->buffer );
			return;
		}
		temp = temp->next_update;
	}
	printf("\n\nupdate not found so cannot be sent.\n\n");
}

//////////////////////////////////////////////////////////////////////////////////
//write an update received from a server to disk.
/////////////////////////////////////////////////////////////////////////////////
void write_update(char *buffer)
{
	int msg_type = ((int *)buffer)[0];

	char filename[] = "x.db";
	filename[0] = svr_id + '0';
	if ((fw = fopen (filename, "a+")) == NULL)
	{
		perror ("fopen");
		exit (0);
	}
	if (msg_type == UPD_READ)
	{
		fwrite((void *)buffer,sizeof(int)*5 + LEN_USER , 1, fw);
	}else if (msg_type == UPD_DEL)
	{
		fwrite((void *)buffer,sizeof(int)*5 + LEN_USER , 1, fw);
	}else if(msg_type == UPD_MSG)
	{
		fwrite((void *)buffer,sizeof(int)*3 + LEN_USER*2 + LEN_SUB + LEN_MSG , 1, fw);
	}else
	{
		printf("error89\n");
	}
	fclose(fw);
	printf("\nupdate written to disk of type %d\n", msg_type);
}

//////////////////////////////////////////////////////////////////////////////////
//insert update into list at correct location.
/////////////////////////////////////////////////////////////////////////////////
void list_update(char *buffer, int type)
{
	int upd_svr =((int *)buffer)[1];
	int upd_id = ((int *)buffer)[2];
	update_list *update = malloc(sizeof(update_list));
	update->update_type = ((int *)buffer)[0];
	update->rec_svr = upd_svr;
	update->msg_id =upd_id;
//	((int *)update->buffer)[0] = type;
	memcpy(update->buffer, buffer, MAX_MSG);
	update->next_update = NULL;
	svr_know[svr_id][upd_svr] = upd_id;
	svr_know[upd_svr][upd_svr] = upd_id;

	update_list *curr = head_update;
	update_list *prev = NULL;

	if (tail_update == NULL)
	{
		tail_update = update;
		head_update = update;
		void print_list();

		return;
	}

	while (curr != NULL)
	{
		if (upd_id > curr-> msg_id)
		{
			prev = curr;
			curr = curr->next_update;
		}else if(upd_id == curr->msg_id)
		{
			if(curr->rec_svr == upd_svr)
			{
				printf("received duplicate update");
				return;
			}else if(curr->rec_svr > upd_svr)
			{
				if(prev == NULL)
				{
					update->next_update = head_update;
					head_update = update;
				}else
				{
					update->next_update = curr;
					prev->next_update = update;
				}
				return;
			}else
			{
				prev = curr;
				curr = curr->next_update;
			}
		}else
		{
			if(prev == NULL)
			{
				update->next_update = head_update;
				head_update = update;
			}else
			{
				update->next_update = curr;
				prev->next_update = update;
			}
			void print_list();

			return;
		}
	}
	prev->next_update = update;
	tail_update = update;
	void print_list();

	printf("\nupdate written to list of type %d\n", ((int *)buffer)[0]);
}

//////////////////////////////////////////////////////////////////////////////
//return 1 if update should be processed, 0 otherwise.
////////////////////////////////////////////////////////////////////////////
int check_update(char *buffer)
{
	if(((int *)buffer)[1] == svr_id)
	{
		return 0;
	}
	//update knowledge matrix and make sure not duplicate message.
	if(svr_know[svr_id][((int *)buffer)[1]] < ((int *)buffer)[2])
	{
		return 1;
	}else
	{
		printf("received an outdated update.\n");
		return 0;
	}
}
////////////////////////////////////////////////////////////////////////////////
//
//  Revsion History
//  10/11/2016 EC: edited the rx and tx functions.
//                 - Added a TRY-CATCH implementation to help CLI parse
//
//
/////////////////////////////////////////////////////////////////////////////////
