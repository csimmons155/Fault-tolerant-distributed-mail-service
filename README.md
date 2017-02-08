# Fault-tolerant-distributed-mail-service
Mail service used by users over a network of computers. Service contains mail server and client applications

<b>Process:</b>

1. Given the menu of valid commands, the user enters the command and, if that command requires a message to be sent to the server, the appropriate message type will generated and the update will be sent. 
2. When the server receives a message from the client, it will stamp it, write it to file, update its linked lists and matrix, and send the update to all other servers in its group. 
3. Other servers will receive the update, look at its timestamp, and if it is higher than the local stamp index in the matrix, then it writes the update to file and updates its data structures. 
4. The first read of an email, all deletes, and all emails are stored as updates and are written to the file upon creation or receipt.  On start-up the servers read and process each update before connecting to spread.  During a periodic sending of the matrix view this file is also cleaned -- removing all unnecessary emails, deletes, and reads below the ARU.  

<b>Client Structure:</b><br>
Clients maintain a linked list of email headers received from the server. 
When the user requests the list of email headers, the client displays the top 10 header nodes in the linked list at a time. 
When the user want to see the next 10, the client displays the next 10 headers within its linked list. 

<b>Server Structure:</b><br>
Servers need a linked list of users, where each user has a linked list of email messages sent to that user. Server also maintains a linked list of updates from itself and other servers. 
Servers stores an int matrix[5][5], where the latest timestamp of updates from each server (from each server’s perspective) is stored. This is maintained in order to resolve merges. 
Servers could build their data structures from reading each update from its update file, storing the updates in a linked list.  The updates read from the file could be read out of order because their order is resolved when they are inserted into the linked list of updates. 


