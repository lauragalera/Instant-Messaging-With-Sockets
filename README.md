# Instant-Messaging-With-Sockets

This instant messaging application (MI) allows the exchange of messages between users following a P2P model and using a protocol of MI addresses. It works on top of the TCP layer and integrates another application called LUMI to localize users by their MI address. LUMI works on top of UDP and follows an architecture Client-Server. There are a set of servers that maintain the information of users in the same domain. This server is iterative and uses the PLUMI protocol and DNS to register, deregister and localize users. These LUMI servers are also called nodes. The MI application exchange protocol messages with its LUMI node using a LUMI agent. This allows localizing users and retrieving their MI addresses to start a conversation. The exchange of messages between users happens in real-time, sending and receiving MI packets that travel in TCP.

![Architecture](architecture.png)

## Agent

* The local user starts the agent application and indicates its @MI (g.e., laura@bb.com), its listening port (g.e., 3500) and nickname.
  * Via LUMI the user is registered in the node of domain bb.com
* The local user wants to talk to a remote user, so she types her @MI (g.e., joya@cc.com). 
  * Via LUMI her @sckMI is found (g.e., 84.88.154.3,TCP, 4456).
* The conversation starts by exchanging messages line by line. These messages use the MI protocol and travel in TCP packets.
* One of the users finishes the conversation.
  * Via LUMI the user is deregistered in the node of domain bb.com.

## Node

* The domain name is the same as the computer's DNS name. 
* A nodelumi.cfg file is read at the beginning and contains the domain name and a list of users who belong to the domain.
* The administrator can register, deregistre and list users from the node's interface.
* The local node talks to remote nodes to find the @MI of a remote user (proxy).

Every time a LUMI agent and LUMI node exchange a message, a new line is added to a log file. The line states if the message was sent or received, @IP and #port UDP remote, the message content, and the number of bytes.

To enhance the application, it has a new feature that displays a message explaining why a register, deregister, or localization has not been possible. Moreover, if an online user receives five localization petitions without the agent ever returning a message, it is considered he went offline unexpectedly. 

The PDF in this repository explains the architecture and its implementation in more detail. It is in Catalan.

