
#include "smpp.h"

extern int  smpp34_errno;
extern char smpp34_strerror[2048];

static int recv_and_unpack(socket_t *sock, void *res, unsigned char *buffer, unsigned int buffer_len);
static int pack_and_send(  socket_t *sock, void *req, unsigned char *buffer, unsigned int buffer_len);

static int pack_and_send(socket_t *sock, void *req, unsigned char *buffer, unsigned int buffer_len){
    int ret = 0;

    //Linealize PDU to buffer
    if((ret = smpp34_pack2( buffer, sizeof(buffer), (int*)&buffer_len, (void*)req)) != 0){
        ERROR(LOG_FILE | LOG_SCREEN, "Error in smpp34_pack():%d:\n%s\n", smpp34_errno, smpp34_strerror)
        return (int)-1;
    }
    if(log_get_display() >= LOG_INFO){
        unsigned char print_buffer[2048] = { 0 };
        //Print PDU
        if((ret = smpp34_dumpPdu2(print_buffer, sizeof(print_buffer), (void*)req)) != 0){
            ERROR(LOG_FILE | LOG_SCREEN, "Error in smpp34_dumpPdu():%d:\n%s\n", smpp34_errno, smpp34_strerror)
            return (int)-1;
        }
        INFO(LOG_SCREEN, "-----------------------------------------------------------\n"
                         "SENDING PDU \n%s\n", print_buffer)
        //Print Buffer
        memset(print_buffer, 0, sizeof(print_buffer));
        if((ret = smpp34_dumpBuf(print_buffer, sizeof(print_buffer), buffer, buffer_len)) != 0){
            ERROR(LOG_FILE | LOG_SCREEN, "Error in smpp34_dumpBuf():%d:\n%s\n", smpp34_errno, smpp34_strerror )
            return (int)-1;
        }
        INFO(LOG_SCREEN, "SENDING BUFFER \n%s\n"
                         "-----------------------------------------------------------\n",print_buffer);
    }
    //Write to socket
    return do_tcp_send(sock, buffer, buffer_len, 0);
}

static int recv_and_unpack(socket_t *sock, void *res, unsigned char *buffer, unsigned int buffer_len){
    int ret = 0;
    int n   = 0;

    //Read from socket
    memset(buffer, 0, sizeof(unsigned char)*buffer_len);
    if((ret = do_tcp_recv(sock, buffer, buffer_len, 0)) <= 0){
        ERROR(LOG_FILE | LOG_SCREEN, "SMPP recv failed")
        return (int)-1;
    }
    
    if(log_get_display() >= LOG_INFO){
        unsigned char print_buffer[2048] = { 0 };
        //Print Buffer
        if((n = smpp34_dumpBuf(print_buffer, sizeof(print_buffer), buffer, buffer_len)) != 0){
            ERROR(LOG_FILE | LOG_SCREEN, "Error in smpp34_dumpBuf():%d:\n%s\n", smpp34_errno, smpp34_strerror )
            return (int)-1;
        }
        INFO(LOG_SCREEN, "-----------------------------------------------------------\n"
                         "RECEIVE BUFFER \n%s\n", print_buffer);
    }
    //Unpack PDU
    if((n = smpp34_unpack2((void*)res, buffer, buffer_len)) != 0){
        ERROR(LOG_FILE | LOG_SCREEN, "Error in smpp34_unpack():%d:%s\n", smpp34_errno, smpp34_strerror)
        return (int)-1;
    }
    if(log_get_display() >= LOG_INFO){
        unsigned char print_buffer[2048] = { 0 };
        //Print PDU
        if((n = smpp34_dumpPdu2(print_buffer, sizeof(print_buffer), (void*)res)) != 0){
            ERROR(LOG_FILE | LOG_SCREEN, "Error in smpp34_dumpPdu():%d:\n%s\n", smpp34_errno, smpp34_strerror);
            return (int)-1;
        }
        INFO(LOG_SCREEN, "RECEIVE PDU \n%s\n"
                         "-----------------------------------------------------------\n", print_buffer)
    }
    return (int)ret;
}

int do_smpp_bind(socket_t *sock, int bind, unsigned char *user, unsigned char *passwd, unsigned char *system_type, int ton_src, int npi_src){
    unsigned char buffer[2048] = { 0 };
    unsigned int  buffer_len   = sizeof(buffer);
    bind_receiver_t      req;
    bind_receiver_resp_t res;
    memset(&req, 0, sizeof(bind_receiver_t));
    memset(&res, 0, sizeof(bind_receiver_resp_t));

    // Init PDU
    req.command_length   = 0;
    req.command_id       = bind;
    req.command_status   = ESME_ROK;
    req.sequence_number  = 1;
    memcpy((void*)req.system_id,(void*)user,sizeof(req.system_id));
    memcpy((void*)req.password,(void*)passwd,sizeof(req.password));
    memcpy((void*)req.system_type,(void*)system_type,sizeof(req.system_type));
    req.interface_version = SMPP_VERSION;
    req.addr_ton          = ton_src;
    req.addr_npi          = npi_src;
    //memcpy((void*)req.address_range(void*)address_range,sizeof(req.address_range));

    if(pack_and_send(sock, (void*)&req, buffer, buffer_len) == -1){
        return (int)-1;
    }

    if(recv_and_unpack(sock, (void*)&res, buffer, buffer_len) == -1){
        return (int)-1;
    }
    destroy_tlv(res.tlv);

    if(res.command_id != (bind | RESP) || res.command_status != ESME_ROK){
        ERROR(LOG_SCREEN | LOG_FILE, "Error in BIND(BIND_RECEIVER)[%d:%d]\n", res.command_id, res.command_status)
        return(int)-1;
    }
    return (int)0;
}
int do_smpp_bind_transceiver(socket_t *sock, unsigned char *user, unsigned char *passwd, unsigned char *system_type, int ton_src, int npi_src){
    unsigned char buffer[2048] = { 0 };
    unsigned int  buffer_len   = sizeof(buffer);
    bind_transceiver_t      req;
    bind_transceiver_resp_t res;
    memset(&req, 0, sizeof(bind_transceiver_t));
    memset(&res, 0, sizeof(bind_transceiver_resp_t));

    // Init PDU
    req.command_length   = 0;
    req.command_id       = BIND_TRANSCEIVER;
    req.command_status   = ESME_ROK;
    req.sequence_number  = 1;
    memcpy((void*)req.system_id,(void*)user,sizeof(req.system_id));
    memcpy((void*)req.password,(void*)passwd,sizeof(req.password));
    memcpy((void*)req.system_type,(void*)system_type,sizeof(req.system_type));
    req.interface_version = SMPP_VERSION;
    req.addr_ton          = ton_src;
    req.addr_npi          = npi_src;
    //memcpy((void*)req.address_range(void*)address_range,sizeof(req.address_range));

    if(pack_and_send(sock, (void*)&req, buffer, buffer_len) == -1){
        return (int)-1;
    }
    if(recv_and_unpack(sock, (void*)&res, buffer, buffer_len) == -1){
        return (int)-1;
    }
    destroy_tlv(res.tlv);

    if(res.command_id != BIND_TRANSCEIVER_RESP || res.command_status != ESME_ROK){
        ERROR(LOG_SCREEN | LOG_FILE, "Error in BIND(BIND_TRANSCEIVER)[%d:%d]\n", res.command_id, res.command_status)
        return(int)-1;
    }
    return (int)0;
}

int do_smpp_bind_receiver(socket_t *sock, unsigned char *user, unsigned char *passwd, unsigned char *system_type, int ton_src, int npi_src){
    unsigned char buffer[2048] = { 0 };
    unsigned int  buffer_len   = sizeof(buffer);
    bind_receiver_t      req;
    bind_receiver_resp_t res;
    memset(&req, 0, sizeof(bind_receiver_t));
    memset(&res, 0, sizeof(bind_receiver_resp_t));

    // Init PDU
    req.command_length   = 0;
    req.command_id       = BIND_RECEIVER;
    req.command_status   = ESME_ROK;
    req.sequence_number  = 1;
    memcpy((void*)req.system_id,(void*)user,sizeof(req.system_id));
    memcpy((void*)req.password,(void*)passwd,sizeof(req.password));
    memcpy((void*)req.system_type,(void*)system_type,sizeof(req.system_type));
    req.interface_version = SMPP_VERSION;
    req.addr_ton          = ton_src;
    req.addr_npi          = npi_src;
    //memcpy((void*)req.address_range(void*)address_range,sizeof(req.address_range));

    if(pack_and_send(sock, (void*)&req, buffer, buffer_len) == -1){
        return (int)-1;
    }

    if(recv_and_unpack(sock, (void*)&res, buffer, buffer_len) == -1){
        return (int)-1;
    }
    destroy_tlv(res.tlv);

    if(res.command_id != BIND_RECEIVER_RESP || res.command_status != ESME_ROK){
        ERROR(LOG_SCREEN | LOG_FILE, "Error in BIND(BIND_RECEIVER)[%d:%d]\n", res.command_id, res.command_status)
        return(int)-1;
    }
    return (int)0;
}


int do_smpp_bind_transmitter(socket_t *sock, unsigned char *user, unsigned char *passwd, unsigned char *system_type, int ton_src, int npi_src){
    unsigned char buffer[2048] = { 0 };
    unsigned int  buffer_len   = sizeof(buffer);
    bind_transmitter_t      req;
    bind_transmitter_resp_t res;
    memset(&req, 0, sizeof(bind_transmitter_t));
    memset(&res, 0, sizeof(bind_transmitter_resp_t));

    /* Init PDU ***********************************************************/
    req.command_length   = 0;
    req.command_id       = BIND_TRANSMITTER;
    req.command_status   = ESME_ROK;
    req.sequence_number  = 1;

    memcpy((void*)req.system_id,(void*)user,sizeof(req.system_id));
    memcpy((void*)req.password,(void*)passwd,sizeof(req.password));
    memcpy((void*)req.system_type,(void*)system_type,sizeof(req.system_type));
    req.interface_version = SMPP_VERSION;
    req.addr_ton          = ton_src;
    req.addr_npi          = npi_src;
    //memcpy((void*)req.address_range(void*)address_range,sizeof(req.address_range));

    if(pack_and_send(sock, (void*)&req, buffer, buffer_len) == -1){
        return (int)-1;
    }
    if(recv_and_unpack(sock, (void*)&res, buffer, buffer_len) == -1){
        return (int)-1;
    }
    destroy_tlv(res.tlv);

    if(res.command_id != BIND_TRANSMITTER_RESP || res.command_status != ESME_ROK){
        ERROR(LOG_SCREEN | LOG_FILE, "Error in BIND(BIND_TRANSMITTER)[%d:%d]\n", res.command_id, res.command_status)
        return (int)-1;
    }
    return (int)0;
}


int do_smpp_send_sms(socket_t *sock, unsigned char *from_msisdn, unsigned char *to_msisdn, unsigned char *message, int ton_src, int npi_src, int ton_dst, int npi_dst){
    if(sock && from_msisdn && to_msisdn && message){
      unsigned char buffer[2048] = { 0 };
      unsigned int  buffer_len   = sizeof(buffer);
      submit_sm_t      req = { 0 };
      submit_sm_resp_t res = { 0 };
      tlv_t tlv = { 0 };
//      memset(&tlv, 0, sizeof(tlv_t));
//      memset(&req, 0, sizeof(submit_sm_t));
//      memset(&res, 0, sizeof(submit_sm_resp_t));

      //Init PDU
      req.command_length   = 0;
      req.command_id       = SUBMIT_SM;
      req.command_status   = ESME_ROK;
      req.sequence_number  = 2;
      
      req.source_addr_ton  = ton_src;
      req.source_addr_npi  = npi_src;
      sprintf((char*)req.source_addr,"%s",(char*)from_msisdn);

      req.dest_addr_ton    = ton_dst;
      req.dest_addr_npi    = npi_dst;
      sprintf((char*)req.destination_addr,"%s",(char*)to_msisdn);
      //if you need this
        //req.esm_class        = 0;
        //req.protocol_id      = 0;
        //req.priority_flag    = 0;
        //snprintf( req.schedule_delivery_time, TIME_LENGTH, "%s", "");
        //snprintf( req.validity_period, TIME_LENGTH, "%s", "");
        //req.registered_delivery = 0;
        //req.replace_if_present_flag =0;
        //req.data_coding         = 0;
        //req.sm_default_msg_id   = 0;

      //if message is <255 octets
        //req.sm_length = strlen((char*)message);
        //memcpy(req.short_message, message, req.sm_length);
      //if message is > 254 octets or > 0 octet
      tlv.tag = TLVID_message_payload;
      tlv.length = strlen((char*)message);
      memcpy(tlv.value.octet, message, tlv.length);
      build_tlv( &(req.tlv), &tlv );

      // Add other optional param - sample
      //tlv.tag = TLVID_user_message_reference;
      //tlv.length = sizeof(uint16_t);
      //tlv.value.val16 = 0x0024;
      //build_tlv( &(req.tlv), &tlv );

      if(pack_and_send(sock, (void*)&req, buffer, buffer_len) == -1){
          return (int)-1;
      }
      destroy_tlv(req.tlv);

      if(recv_and_unpack(sock, (void*)&res, buffer, buffer_len) == -1){
          return (int)-1;
      }

      if(res.command_id != SUBMIT_SM_RESP || res.command_status != ESME_ROK){
          ERROR(LOG_SCREEN | LOG_FILE ,"Error in send(SUBMIT_SM)[%08X:%08X]\n", res.command_id, res.command_status)
          return (int)-1;
      }
      return (int)0;
    }
    return (int)-1;
}

int do_smpp_receive_sms(socket_t *sock, unsigned char **from_msisdn, unsigned char **to_msisdn, unsigned char **message){
    if(from_msisdn && *from_msisdn == NULL && to_msisdn && *to_msisdn == NULL && message && *message == NULL){
        unsigned char buffer[2048] = { 0 };
        unsigned int  buffer_len   = sizeof(buffer);
        int ret = 0;
        data_sm_t       req;
        data_sm_resp_t  res;
        memset(&res, 0, sizeof(data_sm_resp_t));
        memset(&req, 0, sizeof(data_sm_t));
        res.command_length = 0;
        res.command_id = ESME_ROK;
     
        if((ret = recv_and_unpack(sock, (void*)&req, buffer, buffer_len)) <= 0){
            return (int)-1;
        }

        switch(req.command_id){
            case DELIVER_SM :
            {   //copy SMS
                deliver_sm_t *deliver = (deliver_sm_t*)&req;
                if(strlen((char*)deliver->source_addr)>0 && strlen((char*)deliver->destination_addr)>0 && strlen((char*)deliver->short_message)>0){
                    *from_msisdn = (unsigned char*)calloc(strlen((char*)deliver->source_addr)+1,sizeof(char));
                    strcpy((char*)*from_msisdn,(char*)deliver->source_addr);

                    *to_msisdn = (unsigned char*)calloc(strlen((char*)deliver->destination_addr)+1,sizeof(char));
                    strcpy((char*)*to_msisdn,(char*)deliver->destination_addr);

                    *message = (unsigned char*)calloc(strlen((char*)deliver->short_message)+1,sizeof(char));
                    strcpy((char*)*message,(char*)deliver->short_message);
                }else{
                    ERROR(LOG_SCREEN | LOG_FILE ,"Error invalid SMS")
                }
                res.command_id = DELIVER_SM_RESP;
            }
                break;
            case UNBIND :
                //TODO
                res.command_id = UNBIND_RESP;
                break;
            case ENQUIRE_LINK :
                res.command_id = ENQUIRE_LINK_RESP;
                break;
            default:
                res.command_id = DATA_SM_RESP;
                ERROR(LOG_SCREEN | LOG_FILE ,"Request not implemented [%d:%d]", req.command_id, req.command_status)
                break;
        }

        res.sequence_number = req.sequence_number;
        if((ret = pack_and_send(sock, (void*)&res, buffer, buffer_len)) == -1){
            ERROR(LOG_SCREEN | LOG_FILE ,"Error in SM_RESP")
            return (int)-1;
        }
        return (int)req.command_id;
    }
    return (int)-1;
}

int do_smpp_unbind(socket_t *sock){
    unsigned char buffer[2048] = { 0 };
    unsigned int  buffer_len   = sizeof(buffer);
    unbind_t      req;
    unbind_resp_t res;
    memset(&req, 0, sizeof(unbind_t));
    memset(&res, 0, sizeof(unbind_resp_t));

    // Init PDU
    req.command_length   = 0;
    req.command_id       = UNBIND;
    req.command_status   = ESME_ROK;
    req.sequence_number  = 3;

    if(pack_and_send(sock, (void*)&req, buffer, buffer_len) == -1){
        return (int)-1;
    }
    //destroy_tlv(req.tlv);

    if(recv_and_unpack(sock, (void*)&res, buffer, buffer_len) == -1){
        return (int)-1;
    }

    if(res.command_id != UNBIND_RESP || res.command_status != ESME_ROK){
        ERROR(LOG_SCREEN | LOG_FILE ,"Error in send(UNBIND)[%d:%d]\n", res.command_id, res.command_status)
        return (int)-1;
    }

    return (int)0;
}


