#include "callback_stream_analyze.h"

void callback_stream_analyze(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
	/* how many consecutive good/bad packets needed to trigger a reset */
	const int THRESHOLD = 3;

	static int count = 1;
	static int bad_count = 0, good_count = 0;
	static struct tcp_header *tcp_prev = NULL;
	static unsigned int prev_len = 0; 

	struct tcp_header *tcp_pack;
	int hdr_size = SIZE_IP;
	unsigned int sequence, prevseq;

	if (*arg == (u_char) 'f' && tcp_prev != NULL) {
		free(tcp_prev);
		tcp_prev = NULL;
	} else {

		time_analysis(absStartTime, (long int)(pkthdr->ts.tv_sec), (long int)(pkthdr->ts.tv_usec), (int)(pkthdr->len), (int)(pkthdr->caplen));

		if (*arg == (u_char) 'e') {
			hdr_size += SIZE_ETHERNET;
		} else if (*arg == (u_char) 'w') {
			hdr_size += SIZE_WLAN;
		}

		tcp_pack = (struct tcp_header*)(packet + hdr_size);

		if (tcp_prev == NULL) {
			tcp_prev = malloc(sizeof(struct tcp_header));
		} else {
			/* add length of link layer header to the IP header */

			sequence = ntohl(tcp_pack->seq);
			prevseq = ntohl(tcp_prev->seq);

			if (sequence < prevseq) {
				/* SEQ is lower... retransmission likely */	
				good_count = 0;
				bad_count++;
/*				printf("TCP retransmission: packet [%d]\n", count);  */
			} else if (sequence == prevseq) {
				/* SEQ is unchanged... retransmission possible*/
				good_count = 0;
				bad_count++;

/*				printf("TCP retransmission: packet [%d]\n", count);  */
				/* TODO: check previous length and maybe flags to confirm errors! */
				/* or not, it seems to be working okay */
			} else {
				good_count++;
				/* reset bad counter if we've seen enough good packets in a row */
				if (good_count > THRESHOLD) {
					/* if there are many errors in a row, that's a bad sign */
					if (bad_count > THRESHOLD) { 
						printf("TCP Retransmission: packet [%d] (run of %d errors)\n", count-THRESHOLD, bad_count); 
					} 

					bad_count = 0;
				}
			}

			/* Check for TCP reset */
			if ((tcp_pack->tcp_flags & TCP_RST) == TCP_RST) {
				printf("TCP Reset: packet [%d] (possibly bad)\n", count+1);
			}

			/*			printf("seq: %u\tack: %u (prev)\n", ntohl(tcp_prev->seq), ntohl(tcp_prev->ack)); */
			/*			printf("\nseq: %u\tack: %u\n", sequence,  ntohl(tcp_pack->ack)); */
			/*			printf("errors: %d - %d\n\n", err_min, err_max); */
			count++;
		}

		memcpy(tcp_prev, tcp_pack, sizeof(struct tcp_header));
		prev_len = pkthdr->len;
	}
}
