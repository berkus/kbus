/* Kbus kernel module external headers
 *
 * This file provides the definitions (datastructures and ioctls) needed to
 * communicate with the KBUS character device driver.
 */

/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the KBUS Lightweight Linux-kernel mediated
 * message system
 *
 * The Initial Developer of the Original Code is Kynesim, Cambridge UK.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Kynesim, Cambridge UK
 *   Tony Ibbs <tony.ibbs@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of the
 * GNU Public License version 2 (the "GPL"), in which case the provisions of
 * the GPL are applicable instead of the above.  If you wish to allow the use
 * of your version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file under either
 * the MPL or the GPL. 
 *
 * ***** END LICENSE BLOCK *****
 */

#ifndef _kbus_defns
#define _kbus_defns

#if ! __KERNEL__ && defined(__cplusplus)
extern "C" {
#endif

#if __KERNEL__
#include <linux/kernel.h>
#include <linux/ioctl.h>
#else
#include <stdint.h>
#include <sys/ioctl.h>
#endif

/* A message id is made up of two fields.
 *
 * If the network id is 0, then it is up to us (KBUS) to assign the
 * serial number. In other words, this is a local message.
 *
 * If the network id is non-zero, then this message is presumed to
 * have originated from another "network", and we preserve both the
 * network id and the serial number.
 *
 * The message id {0,0} is special and reserved (for use by KBUS).
 */
struct kbus_msg_id {
	uint32_t		network_id;
	uint32_t		serial_num;
};

/* When the user asks to bind a message name to an interface, they use: */
struct kbus_bind_struct {
	uint32_t	 is_replier;	/* are we a replier? */
	uint32_t	 name_len;
	char		*name;
};

/* When the user requests the id of the replier to a message, they use: */
struct kbus_bind_query_struct {
	uint32_t	 return_id;
	uint32_t	 name_len;
	char		*name;
};

/* When the user writes/reads a message, they use: */
struct kbus_message_struct {
	/*
	 * The guards
	 * ----------
	 *
	 * * 'start_guard' is notionally "kbus", and 'end_guard' (the 32 bit
	 *   word after the rest of the message) is notionally "subk". Obviously
	 *   that depends on how one looks at the 32-bit word. Every message
	 *   shall start with a start guard and end with an end guard.
	 *
	 * These provide some help in checking that a message is well formed,
	 * and in particular the end guard helps to check for broken length
	 * fields.
	 *
	 * The message header
	 * ------------------
	 *
	 * - 'id' identifies this particular message.
	 *
	 *   When writing a new message, set this to 0 (actually, KBUS will
	 *   entirely ignore this field, as it set it itself).
	 *
	 *   When reading a message, this will have been set by KBUS.
	 *
	 *   When replying to a message, copy this value into the 'in_reply_to'
	 *   field, so that the recipient will know what message this was a
	 *   reply to.
	 *
	 * - 'in_reply_to' identifies the message this is a reply to.
	 *
	 *   This shall be set to 0 unless this message *is* a reply to a
	 *   previous message. In other words, if this value is non-0, then
	 *   the message *is* a reply.
	 *
	 * - 'to' is who the message is to be sent to.
	 *
	 *   When writing a new message, this should normally be set to 0,
	 *   meaning "anyone listening" (but see below if "state" is being
	 *   maintained).
	 *
	 *   When replying to a message, it shall be set to the 'from' value
	 *   of the orginal message.
	 *
	 *   When constructing a request message (a message wanting a reply),
	 *   then it can be set to a specific listener id. When such a message
	 *   is sent, if the replier bound (at that time) does not have that
	 *   specific listener id, then the send will fail.
	 *
	 * - 'from' indicates who sent the message.
	 *
	 *   When writing a new message, set this to 0.
	 *
	 *   When reading a message, this will have been set by KBUS.
	 *
	 *   When replying to a message, put the value read into 'to',
	 *   and set 'from' to 0 (see the "hmm" caveat under 'to' above,
	 *   though).
	 *
	 *       Question: if a reply is not wanted, should this be 0?
	 *
	 * - 'flags' indicates the type of message.
	 *
	 *   When writing a message, this can be used to indicate that
	 *
	 *   * the message is URGENT
	 *   * a reply is wanted
	 *
	 *   When reading a message, this indicates:
	 *
	 *   * the message is URGENT
	 *   * a reply is wanted
	 *
	 *   When writing a reply, set this field to 0 (I think).
	 *
	 * The message body
	 * ----------------
	 *
	 * - 'name_len' is the length of the message name in bytes.
	 *
	 *   When writing a new message, this must be non-zero.
	 *
	 *   When replying to a message, this must be non-zero - i.e., the
	 *   message name must still be given - although it is possible
	 *   that this may change in the future.
	 *
	 * - 'data_len' is the length of the message data, in 32-bit
	 *   words. It may be zero.
	 *
	 * - 'name_and_data' is the bytes of message name immediately
	 *   followed by the bytes of message data.
	 *
	 *   * The message name must be padded out to a multiple of 4 bytes.
	 *     This is not indicated by the message length. Padding should be
	 *     with zero bytes (but it is not necessary for there to be a zero
	 *     byte at the end of the name). Byte ordering is according to that
	 *     of the platform, treating msg->name_and_data[0] as the start of
	 *     an array of bytes.
	 *
	 *   * The data is not touched by KBUS, and may include any values.
	 *
	 * The last element of 'name_and_data' must always be an end guard,
	 * with value 0x7375626B ('subk') (so the *actual* data is always one
	 * 32-bit word shorter than that indicated).
	 */
	uint32_t		start_guard;
	struct kbus_msg_id	id;	     /* Unique to this message */
	struct kbus_msg_id	in_reply_to; /* Which message this is a reply to */
	uint32_t		to;	     /* 0 (normally) or a replier id */
	uint32_t		from;	     /* 0 (KBUS) or the sender's id */
	uint32_t		flags;	     /* Message type/flags */
	uint32_t		name_len;    /* Message name's length, in bytes */
	uint32_t		data_len;    /* Message length, as 32 bit words */
	uint32_t		rest[];	     /* Message name, data and end_guard */
};

#define KBUS_MSG_START_GUARD	0x7375626B
#define KBUS_MSG_END_GUARD	0x6B627573

/*
 * Things KBUS changes in a message
 * --------------------------------
 * In general, KBUS leaves the content of a message alone. However, it does
 * change:
 *
 * - the message id (if id.network_id is unset)
 * - the from id (to indicate the KSock this message was sent from)
 * - the KBUS_BIT_WANT_YOU_TO_REPLY bit in the flags (set or cleared
 *   as appropriate)
 */

/*
 * Flags for the message 'flags' word
 * ----------------------------------
 * The KBUS_BIT_WANT_A_REPLY bit is set by the sender to indicate that a
 * reply is wanted. This makes the message into a request.
 *
 *     Note that setting the WANT_A_REPLY bit (i.e., a request) and
 *     setting 'in_reply_to' (i.e., a reply) is bound to lead to
 *     confusion, and the results are undefined (i.e., don't do it).
 *
 * The KBUS_BIT_WANT_YOU_TO_REPLY bit is set by KBUS on a particular message
 * to indicate that the particular recipient is responsible for replying
 * to (this instance of the) message. Otherwise, KBUS clears it.
 *
 * The KBUS_BIT_SYNTHETIC bit is set by KBUS when it generates a synthetic
 * message (an exception, if you will), for instance when a replier has
 * gone away and therefore a reply will never be generated for a request
 * that has already been queued.
 *
 *     Note that KBUS does not check that a sender has not set this
 *     on a message, but doing so may lead to confusion.
 *
 * The KBUS_BIT_URGENT bit is set by the sender if this message is to be
 * treated as urgent - i.e., it should be added to the *front* of the
 * recipient's message queue, not the back.
 *
 * Send flags
 * ==========
 * There are two "send" flags, KBUS_BIT_ALL_OR_WAIT and KBUS_BIT_ALL_OR_FAIL.
 * Either one may be set, or both may be unset.
 *
 *    If both bits are set, the message will be rejected as invalid.
 *
 *    Both flags are ignored in reply messages (i.e., messages with the
 *    'in_reply_to' field set).
 *
 * If both are unset, then a send will behave in the default manner. That is,
 * the message will be added to a listener's queue if there is room but
 * otherwise the listener will (silently) not receive the message.
 *
 *     (Obviously, if the listener is a replier, and the message is a request,
 *     then a KBUS message will be synthesised in the normal manner when a
 *     request is lost.)
 *
 * If the KBUS_BIT_ALL_OR_WAIT bit is set, then a send should block until
 * all recipients can be sent the message. Specifically, before the message is
 * sent, all recipients must have room on their message queues for this
 * message, and if they do not, the send will block until there is room for the
 * message on all the queues.
 *
 * If the KBUS_BIT_ALL_OR_FAIL bit is set, then a send should fail if all
 * recipients cannot be sent the message. Specifically, before the message is
 * sent, all recipients must have room on their message queues for this
 * message, and if they do not, the send will fail.
 */

#if ! __KERNEL__
#define BIT(num)                 (((unsigned)1) << (num))
#endif

#define	KBUS_BIT_WANT_A_REPLY		BIT(0)
#define KBUS_BIT_WANT_YOU_TO_REPLY	BIT(1)
#define KBUS_BIT_SYNTHETIC		BIT(2)
#define KBUS_BIT_URGENT			BIT(3)

#define KBUS_BIT_ALL_OR_WAIT		BIT(8)
#define KBUS_BIT_ALL_OR_FAIL		BIT(9)

/*
 * Given name_len (in bytes) and data_len (in 32-bit words), return the
 * length of the appropriate kbus_message_struct, in bytes
 *
 * Don't forget the end guard at the end...
 *
 * Remember that "sizeof" doesn't count the 'rest' field in our message
 * structure.
 *
 * (it's sensible to use "sizeof" so that we don't need to amend the macro
 * if the message datastructure changes)
 */
#define KBUS_MSG_LEN(name_len,data_len)    (sizeof(struct kbus_message_struct) + \
					    4 * ((name_len+3)/4 + data_len + 1))

/*
 * The message name starts at msg->rest[0].
 * The message data starts after the message name - given the message
 * name's length (in bytes), that is at index:
 */
#define KBUS_MSG_DATA_INDEX(name_len)     ((name_len+3)/4)
/*
 * Given the message name length (in bytes) and the message data length (in
 * 32-bit words), the index of the end guard is thus:
 */
#define KBUS_MSG_END_GUARD_INDEX(name_len,data_len)  ((name_len+3)/4 + data_len)

#define KBUS_IOC_MAGIC	'k'	/* 0x6b - which seems fair enough for now */
/*
 * RESET: reserved for future use
 */
#define KBUS_IOC_RESET	  _IO(  KBUS_IOC_MAGIC,  1)
/*
 * BIND - bind a KSock to a message name
 * arg: struct kbus_m_bind_struct, indicating what to bind to
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_BIND	  _IOW( KBUS_IOC_MAGIC,  2, char *)
/*
 * UNBIND - unbind a KSock from a message id
 * arg: struct kbus_m_bind_struct, indicating what to unbind from
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_UNBIND	  _IOW( KBUS_IOC_MAGIC,  3, char *)
/*
 * KSOCKID - determine a KSock's KSock id
 * arg (out): uint32_t, indicating this KSock's internal id
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_KSOCKID  _IOR( KBUS_IOC_MAGIC,  4, char *)
/*
 * REPLIER - determine the KSock id of the replier for a message name
 * arg: struct kbus_m_bind_query_struct
 *
 *    - on input, specify the message name and replier flag to ask about
 *    - on output, KBUS fills in the relevant KSock id in the return_value,
 *      or 0 if there is no bound replier
 *
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_REPLIER  _IOWR(KBUS_IOC_MAGIC,  5, char *)
/*
 * NEXTMSG - pop the next message from the read queue
 * arg (out): uint32_t, number of bytes in the next message, 0 if there is no
 *            next message
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_NEXTMSG  _IOR( KBUS_IOC_MAGIC,  6, char *)
/*
 * LENLEFT - determine how many bytes are left to read of the current message
 * arg (out): uint32_t, number of bytes left, 0 if there is no current read
 *            message
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_LENLEFT  _IOR( KBUS_IOC_MAGIC,  7, char *)
/*
 * SEND - send the current message
 * arg (out): struct kbus_msg_id, the message id of the sent message
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_SEND	  _IOR( KBUS_IOC_MAGIC,  8, char *)
/*
 * DISCARD - discard the message currently being written (if any)
 * arg: none
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_DISCARD  _IO(  KBUS_IOC_MAGIC,  9)
/*
 * LASTSENT - determine the message id of the last message SENT
 * arg (out): struct kbus_msg_id, {0,0} if there was no last message
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_LASTSENT _IOR( KBUS_IOC_MAGIC, 10, char *)
/*
 * MAXMSGS - set the maximum number of messages on a KSock read queue
 * arg (in): uint32_t, the requested length of the read queue, or 0 to just
 *           request how many there are
 * arg (out): uint32_t, the length of the read queue after this call has
 *            succeeded
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_MAXMSGS  _IOWR(KBUS_IOC_MAGIC, 11, char *)
/*
 * NUMMSGS - determine how many messages are in the read queue for this KSock
 * arg (out): uint32_t, the number of messages in the read queue.
 * retval: 0 for success, negative for failure
 */
#define KBUS_IOC_NUMMSGS  _IOR( KBUS_IOC_MAGIC, 12, char *)

/* XXX If adding another IOCTL, remember to increment the next number! XXX */
#define KBUS_IOC_MAXNR	12

#if ! __KERNEL__ && defined(__cplusplus)
}
#endif

#endif // _kbus_defns

// Kernel style layout -- note that including this text contravenes the Linux
// coding style, and is thus a Bad Thing. Expect these lines to be removed if
// this ever gets added to the kernel distribution.
// Local Variables:
// c-set-style: "linux"
// End:
// vim: set tabstop=8 shiftwidth=8 noexpandtab: