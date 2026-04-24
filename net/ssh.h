#ifndef SSH_H
#define SSH_H

/* scorpion OS - net/ssh.h
   Minimal SSH-2 client:
    - TCP connect to port 22
    - Version exchange
    - Key exchange: Diffie-Hellman group14-sha1
    - Host key algo: ssh-rsa (verification skipped — TOFU model)
    - Encryption:    aes128-cbc
    - MAC:           hmac-sha1
    - Auth:          password
    - Channel:       session / pty-req / shell
   Once the encrypted channel is open, keyboard input is forwarded
   to the socket and socket output is fed through terminal_putchar()
   so ANSI sequences render properly.                              */

#include <stdint.h>
#include <stddef.h>

/* Connect to host:22, log in with username/password, open interactive shell.
   Blocks until the remote closes the channel or the user types ~. (tilde dot). */
int ssh_connect(const char *host_ip_str, const char *username, const char *password);

#endif /* SSH_H */
