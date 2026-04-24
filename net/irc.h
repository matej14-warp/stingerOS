#ifndef IRC_H
#define IRC_H

/* scorpion OS - net/irc.h
   Minimal IRC client:
   - TCP connect to server:6667
   - NICK / USER registration
   - JOIN channel
   - PRIVMSG send
   - Display incoming PRIVMSG + server notices
   - /quit to disconnect                       */

int irc_connect(const char *server_ip, const char *nick,
                const char *channel);

#endif
