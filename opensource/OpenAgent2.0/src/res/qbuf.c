/*
 * Copyright(c) 2006-2007, Works Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution. 
 * 3. Neither the name of the vendors nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/***********************************************************************
*
*	include file
*
***********************************************************************/
#include "qbuf.h"

int buf_queue_create(Qbuf_list *, int);
void buf_queue_destroy(Qbuf_list *);
int buf_queue_reset_lock(Qbuf_list *);
Qbuf_node* get_buf(Qbuf_list *);
void append_buf(Qbuf_list *, Qbuf_node *);
int get_qbuf_node_num(Qbuf_list *);
int qbuf_node_move(Qbuf_list *, Qbuf_list *);
#ifdef DEBUG_DISPLAY
void buf_queue_display(Qbuf_list *);
#endif

/*
 *********************************************************************
 * Function name: qbuf_get_data
 * Description: get the pointer point to the data buffer in the Qbuf_node
 * Parameter: Qbuf_node *this: the pointer point to itself
 * Return value: the pointer point to the data buffer
 *********************************************************************
*/
char* qbuf_get_data(Qbuf_node *this)
{
    return this->data;
}

/*
 *********************************************************************
 * Function name: qbuf_copy_to_data
 * Description: copy the passed data to the data buffer in the Qbuf_node
 * Parameter: Qbuf_node *this: the pointer point to itself; char *str: the pointer point to the data to be copied
 * Return value: none
 *********************************************************************
*/
void qbuf_copy_to_data(Qbuf_node *this, char *str)
{
    strcpy(this->data, str);
}

/*
 *********************************************************************
 * Function name: qbuf_copy_from_data
 * Description: copy the the data buffer in the Qbuf_node to passed data buffer
 * Parameter: Qbuf_node *this: the pointer point to itself; char *p_str:  the pointer point to the data to get the data buffer data
 * Return value: none
 *********************************************************************
*/
void qbuf_copy_from_data(Qbuf_node *this, char *p_str)
{
    strcpy(p_str, this->data);
}

/*
 *********************************************************************
 * Function name: init_list_member
 * Description: set the function pointer point to the functions and the this pointer point to the "object"
 * Parameter: Qbuf_list *p_list: the pointer to the Qbuf_list "object"
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int init_list_member(Qbuf_list *p_list)
{
    if(pthread_mutex_init(&(p_list->lock), NULL) != 0)
    {
        return FAIL;
    }

    INIT_LIST_HEAD(&(p_list->header));
    p_list->qbuf_node_num = 0;
    p_list->create = buf_queue_create;
    p_list->destroy = buf_queue_destroy;
    p_list->reset_lock = buf_queue_reset_lock;
    p_list->head = get_buf;
    p_list->append = append_buf;
    p_list->qbuf_nodes_num = get_qbuf_node_num;
    p_list->move_node = qbuf_node_move;
#ifdef DEBUG_DISPLAY
    p_list->display = buf_queue_display;
#endif

    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: buf_queue_create
 * Description: create the appointed number nodes
 * Parameter: Qbuf_list *this: the pointer point to itself; int node_num: the number of the nodes want to be created
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int buf_queue_create(Qbuf_list *this, int node_num)
{
    int i;
    Qbuf_node *p_temp_node = NULL;
    int result;

    this->qbuf_node_num = node_num;	

    for(i = 0; i < node_num; i++)
    {
        p_temp_node = NULL;
		
        p_temp_node = (Qbuf_node*)malloc(sizeof(Qbuf_node));
        if(p_temp_node == NULL)
        {
            result = FAIL;
            return result;
        }

        INIT_LIST_HEAD(&(p_temp_node->next));
        p_temp_node->get_data_ptr = qbuf_get_data;
        p_temp_node->set = qbuf_copy_to_data;
        p_temp_node->copy = qbuf_copy_from_data;
        #ifdef DEBUG_DISPLAY
        p_temp_node->set(p_temp_node, "");
        #endif
        list_add_tail(&(p_temp_node->next), &(this->header));
    }

    result = SUCCESS;

    return result;
}

/*
 *********************************************************************
 * Function name: buf_queue_destroy
 * Description: destory the nodes in buffer queue
 * Parameter: Qbuf_list *this: the pointer point to itself;
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
void buf_queue_destroy(Qbuf_list *this)
{
    int i = 0;
    Qbuf_node *p_temp_node = NULL;

    pthread_mutex_lock(&(this->lock));

    for(i = 0; i < this->qbuf_node_num; i++)
    {
        p_temp_node = list_entry((&(this->header))->next, Qbuf_node, next);
        list_del(&(p_temp_node->next));

        free(p_temp_node);
    }

    this->qbuf_node_num = 0;

    pthread_mutex_unlock(&(this->lock));
    pthread_mutex_destroy(&(this->lock));
}

/*
 *********************************************************************
 * Function name: buf_queue_reset_lock
 * Description: reset the lock
 * Parameter: Qbuf_list *this: the pointer point to itself;
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int buf_queue_reset_lock(Qbuf_list *this)
{
    pthread_mutex_unlock(&(this->lock));
    pthread_mutex_destroy(&(this->lock));
    if( pthread_mutex_init(&(this->lock), NULL) != 0 )
    {
        return FAIL;
    }
    
    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: get_buf
 * Description: get and delete the first node in buffer queue
 * Parameter: Qbuf_list *this: the pointer point to itself;
 * Return value: the pointer point to the got node
 *********************************************************************
*/
Qbuf_node* get_buf(Qbuf_list *this)
{
    Qbuf_node *p_temp_node = NULL;
    Qbuf_node *p_result_node = NULL;

    pthread_mutex_lock(&(this->lock));

    if(this->qbuf_node_num <= 0)
    {
        p_result_node = NULL;
    }
    else
    {
        p_temp_node = list_entry((&(this->header))->next, Qbuf_node, next);


        list_del(&(p_temp_node->next));
        this->qbuf_node_num--;

        p_result_node = p_temp_node;
    }

    pthread_mutex_unlock(&(this->lock));
    return p_result_node;
}

/*
 *********************************************************************
 * Function name: append_buf
 * Description: append a node to the tail of buffer queue
 * Parameter: Qbuf_list *this: the pointer point to itself; Qbuf_node *p_node: the pointer point to the node to be appended
 * Return value: none
 *********************************************************************
*/
void append_buf(Qbuf_list *this, Qbuf_node *p_node)
{
    pthread_mutex_lock(&(this->lock));

    list_add_tail(&(p_node->next), &(this->header));
    this->qbuf_node_num++;

    pthread_mutex_unlock(&(this->lock));
}

/*
 *********************************************************************
 * Function name: get_qbuf_node_num
 * Description: get the number of nodes in the buffer queue
 * Parameter: Qbuf_list *this: the pointer point to itself;
 * Return value: the number of nodes in the buffer queue
 *********************************************************************
*/
int get_qbuf_node_num(Qbuf_list *this)
{
    int result;

    pthread_mutex_lock(&(this->lock));
    result = this->qbuf_node_num;
    pthread_mutex_unlock(&(this->lock));

    return result;
}

/*
 *********************************************************************
 * Function name: qbuf_node_move
 * Description: move a node form one buffer queue to another buffer queue
 * Parameter: Qbuf_list *this: the pointer point to itself; Qbuf_list *dest_list: the pointer point to the destination buffer queue
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int qbuf_node_move(Qbuf_list *this, Qbuf_list *dest_list)
{
    Qbuf_node *p_temp_node = NULL;
    int result;

    p_temp_node = this->head(this);

    if(!p_temp_node)
    {
        result = FAIL;
    }
    else
    {
        dest_list->append(dest_list, p_temp_node);

        result = SUCCESS;
    }

    return result;
}

#ifdef DEBUG_DISPLAY
/*
 *********************************************************************
 * Function name: buf_queue_display
 * Description: display some info in buffer queue(for debugging)
 * Parameter: Qbuf_list *this: the pointer point to itself;
 * Return value: none
 *********************************************************************
*/
void buf_queue_display(Qbuf_list *this)
{
    Qbuf_node *p_temp_node = NULL;
    struct list_head *p_temp_head = NULL;
    int i = 1;

    LOG(m_res, DEBUG, "\n******Nodes in list begin******\n");

    list_for_each(p_temp_head, &(this->header))
    {
        p_temp_node = list_entry(p_temp_head, Qbuf_node, next);
        //LOG(m_res, DEBUG, "node%d  :  %s\n", i, p_temp_node->data);
        i++;
    }
    LOG(m_res, DEBUG, "node count : %d\n", i - 1); 
    LOG(m_res, DEBUG, "****** Nodes in list end ******\n");
}
#endif
