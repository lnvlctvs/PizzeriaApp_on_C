#include "pizza.h"
#include <pthread.h>
#include <stdbool.h>

//conditions
pthread_cond_t callReciever_cond;			//cond gia tilefonites
pthread_cond_t cook_cond;					//cond gia paraskeuastes
pthread_cond_t oven_cond;					//cond gia fournous
pthread_cond_t packer_cond;					//cond gia ton paketa
pthread_cond_t deliverer_cond;				//cond gia dianomis

//mutexes
pthread_mutex_t callReciever_mtx ;			//mutex gia ton tilefoniti
pthread_mutex_t cook_mtx;					//mutex gia paraskeuastes
pthread_mutex_t oven_mtx;					//mutex gia fournous
pthread_mutex_t packer_mtx;					//mutex gia ton paketa
pthread_mutex_t deliverer_mtx;				//mutex gia dianomis
pthread_mutex_t screen_mtx;					//mutex gia othoni
pthread_mutex_t total_money_mtx;			//mutex gia ta sinolika esoda
pthread_mutex_t complete_delivery_mtx;		//mutex gia ton arithmo ton olokliromenon paraggelion
pthread_mutex_t max_waiting_time_mtx;		//mutex gia megisto xrono anamonis
pthread_mutex_t max_total_time_mtx;			//mutex gia megisto xrono oloklirosis
pthread_mutex_t max_cooling_time_mtx;		//mutex gia megisto xrono kriomatos
pthread_mutex_t average_waiting_time_mtx;	//mutex gia meso xrono anamonis
pthread_mutex_t average_total_time_mtx;		//mutex gia meso xrono oloklirosis
pthread_mutex_t average_cooling_time_mtx;	//mutex gia meso xrono kriomatos


//variables
int callRecievers = NCALLRECIEVERS;
int cooks = NCOOK;
int ovens = NOVEN;
int packer = NPACKERS;
int deliverers = NDELIVERER;
int total_income = 0;
int total_completed_deliveries = 0;
int max_waiting_time = 0;
int max_total_time = 0;
int max_cooling_time = 0;
int average_waiting_time = 0;
int average_total_time = 0;
int average_cooling_time = 0;
int seed;

// calculating probability of charging successfully the credit card of the client
bool prob_true(double p){
    return rand()/(RAND_MAX+1.0) < p;
}

//fuction called by the threads
void *pizzaOrder(void * customer){

	//variables used for the statistics
	struct timespec order_start, order_answered, pizza_baked,order_packaged, order_delivered;
	int time;

	//order's id
	int id = *(int *) customer;
	int rc;
	int numOfPizzas;
	int waiting_time;
	int charging_time;
	int packaging_time;
	int delivery_time;
	int pizza_service_time;
	int cooling_time;

	//internal seed
	unsigned int local_seed;
	local_seed = seed - id;


	//--------------------------DELIVERY THROUGH PHONE CALL BEGINS HERE---------------------------
	
	//getting the timestamp when the client first arrived
	clock_gettime(CLOCK_REALTIME, &order_start);

	pthread_mutex_lock(&callReciever_mtx);
	while(callRecievers==0){
		pthread_cond_wait(&callReciever_cond, &callReciever_mtx);
	}
	callRecievers = callRecievers - 1;
	pthread_mutex_unlock(&callReciever_mtx);
	
	//getting the timestamp when the call reciever answered to the client
	clock_gettime(CLOCK_REALTIME, &order_answered);

	//number of pizzas
	numOfPizzas = rand_r(&local_seed) % NORDERHIGH + NORDERLOW;

	//calculating charging time
	charging_time = (rand_r(&local_seed) % TPAYMENTHIGH) + TPAYMENTLOW;

	//waiting for Call Reciever to charge the credit card of the client
	sleep(charging_time);

	bool success;
	success = prob_true(1-PFAIL);
	
	//release the Call Reciever
	pthread_mutex_lock(&callReciever_mtx);
	callRecievers = callRecievers + 1;
	pthread_cond_signal(&callReciever_cond);
	pthread_mutex_unlock(&callReciever_mtx);
	
	
	//----------------------------CALL RECIEVER'S JOB ENDS HERE--------------------------------
	
	if(success)
	{
		pthread_mutex_lock(&screen_mtx);
		printf("The order with number %d was successfully placed into the system and the pizzeria will begin processing it.  \n",id);
		pthread_mutex_unlock(& screen_mtx);
		
		//adding money to the total income
		pthread_mutex_lock(&total_money_mtx);
		total_income = total_income + (numOfPizzas * CPIZZA);
		pthread_mutex_unlock(&total_money_mtx);
		
		//increasing successfull orders
		pthread_mutex_lock(&complete_delivery_mtx);
		total_completed_deliveries = total_completed_deliveries + 1;
		pthread_mutex_unlock(&complete_delivery_mtx);
		

	//------------------------------COOKS JOB STARTS HERE--------------------------------------

		//wait till at least one cook is available
		pthread_mutex_lock(&cook_mtx);
		while(cooks==0){
		pthread_cond_wait(&cook_cond, &cook_mtx);
		}
		cooks = cooks - 1;
		pthread_mutex_unlock(&cook_mtx);

		//wait for pizza preparation
		sleep(TPREP * numOfPizzas);

	//-------------------------------OVENS STARTS HERE------------------------------------------
		pthread_mutex_lock(&oven_mtx);
		while(ovens < numOfPizzas){
			pthread_cond_wait(&oven_cond, &oven_mtx);
		}
		ovens = ovens - numOfPizzas;
		pthread_mutex_unlock(&oven_mtx);

		//release the cook
		pthread_mutex_lock(&cook_mtx);
		cooks = cooks + 1;
		pthread_cond_signal(&cook_cond);
		pthread_mutex_unlock(&cook_mtx);

	//------------------------------COOKS JOB FINISHES HERE--------------------------------------

		//wait for all pizzas to be baked
		sleep(TBAKE);

		//get the timestamp when the pizzas are baked and waiting for the deliverer
		clock_gettime(CLOCK_REALTIME, &pizza_baked);
		
	//-------------------------------PACKER'S JOB STARTS HERE------------------------------------

		//wait till the packer is free
		pthread_mutex_lock(&packer_mtx);
		while(packer==0){
			pthread_cond_wait(&packer_cond, &packer_mtx);
		}
		packer = packer - 1;
		pthread_mutex_unlock(&packer_mtx);
		
		//release the ovens
		pthread_mutex_lock(&oven_mtx);
		ovens = ovens + numOfPizzas;
		pthread_cond_signal(&oven_cond);
		pthread_mutex_unlock(&oven_mtx);
		
	//-----------------------------OVENS RELEASED AND TERMINATE HERE------------------------------
		
		//waiting the packer to pack the pizzas into boxes
		sleep(TPACK);
		
		//getting the timestamp when the packaging of all the pizzas was done
		clock_gettime(CLOCK_REALTIME, &order_packaged);
		
		pthread_mutex_lock(&packer_mtx);
		packer = packer + 1;
		pthread_cond_signal(&packer_cond);
		pthread_mutex_unlock(&packer_mtx);
		
	//----------------------------PACKAGING GUY RELEASED HERE--------------------------------------
		
		packaging_time = order_packaged.tv_sec - order_start.tv_sec;
		
		pthread_mutex_lock(&screen_mtx);
		printf("The order with number %d was successfully packaged in %d minutes .  \n",id,packaging_time);
		pthread_mutex_unlock(& screen_mtx);
				

	//-----------------------------DELIVERY GUY STARTS HERE---------------------------------------
		pthread_mutex_lock(&deliverer_mtx);
		while(deliverers==0){
			pthread_cond_wait(&deliverer_cond, &deliverer_mtx);
		}
		deliverers = deliverers - 1;
		pthread_mutex_unlock(&deliverer_mtx);
		

		//delivery time
		delivery_time = rand_r(&local_seed) % (TDELIVERYHIGH - TDELIVERYLOW + 1) + TDELIVERYLOW;
	
		//wait for the deliverer to get to the customer
		sleep(delivery_time);
		
		//get the timestamp when the pizzas are delivered to the customer
		clock_gettime(CLOCK_REALTIME, &order_delivered);

		//wait for the deliverer to return to the pizzeria
		sleep(delivery_time);

		//release the deliverer
		pthread_mutex_lock(&deliverer_mtx);
		deliverers = deliverers + 1;
		pthread_cond_signal(&deliverer_cond);
		pthread_mutex_unlock(&deliverer_mtx);
		
		pizza_service_time = order_delivered.tv_sec - order_start.tv_sec;
		
		pthread_mutex_lock(&screen_mtx);
		printf("The order with number %d was successfully delivered in %d minutes .\n",id,pizza_service_time);
		pthread_mutex_unlock(& screen_mtx);
		

	//---------------------------DELIVERER'S JOB ENDS HERE----------------------------------------
	
	
	//---------------------SAVING DATA STATISTICS OF THE DELIVERY HERE----------------------------
	

		waiting_time = order_answered.tv_sec - order_start.tv_sec;
		
		//update max and average waiting times
		if(waiting_time > max_waiting_time){
			pthread_mutex_lock(&max_waiting_time_mtx);
			max_waiting_time = waiting_time;
			pthread_mutex_unlock(&max_waiting_time_mtx);
		}
		pthread_mutex_lock(&average_waiting_time_mtx);
		average_waiting_time = average_waiting_time + waiting_time;
		pthread_mutex_unlock(&average_waiting_time_mtx);

		cooling_time = order_delivered.tv_sec - pizza_baked.tv_sec;

		//update max and average cooling times
		if(cooling_time > max_cooling_time){
			pthread_mutex_lock(&max_cooling_time_mtx);
			max_cooling_time = cooling_time;
			pthread_mutex_unlock(&max_cooling_time_mtx);
		}
		pthread_mutex_lock(&average_cooling_time_mtx);
		average_cooling_time = average_cooling_time + cooling_time;
		pthread_mutex_unlock(&average_cooling_time_mtx);

		//update max and average service time
		if(pizza_service_time > max_total_time){
			pthread_mutex_lock(&max_total_time_mtx);
			max_total_time = pizza_service_time;
			pthread_mutex_unlock(&max_total_time_mtx);
		}
		pthread_mutex_lock(&average_total_time_mtx);
		average_total_time = average_total_time + pizza_service_time;
		pthread_mutex_unlock(&average_total_time_mtx);

		return 0;
	
	}else{
		
	pthread_mutex_lock(&screen_mtx);
	printf("The order with number %d was NOT successfully placed into the system.\nCredit card charge failed .\n",id);
	pthread_mutex_unlock(& screen_mtx);
		
	pthread_exit(0);
	}
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//				  										       |
//						M A I N							       |
//					    F U N C T I O N							       |
//														       |
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc ,char * argv[]){

	//main variables
	int numOfClients;
	int rc;
	unsigned int temp;
	double average;

	//main variables' check and initialization
	if(argc != 3) {
		printf("ERROR: the program should take two arguments, the number of clients and the seed!\n");
		exit(-1);
	}
	numOfClients = atoi(argv[1]);
	seed = atoi(argv[2]);

	if (numOfClients <= 0) {
		printf("ERROR: Clients's number must be positive\n");
		exit(-1);
	}


	//-------------------------MUTEX BEIGN INITIALIAZED HERE-----------------------
	rc = pthread_mutex_init(&callReciever_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&cook_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&oven_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&packer_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&deliverer_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&screen_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&total_money_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&complete_delivery_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&max_waiting_time_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&max_total_time_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&max_cooling_time_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&average_waiting_time_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&average_total_time_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_init(&average_cooling_time_mtx, NULL);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_cond_init(&callReciever_cond, NULL);
	if(rc != 0) {
    		printf("ERROR: return code from pthread_cond_init() is %d\n", rc) ;
       		exit(-1);
	}
	rc = pthread_cond_init(&cook_cond, NULL);
	if(rc != 0) {
    		printf("ERROR: return code from pthread_cond_init() is %d\n", rc) ;
       		exit(-1);
	}
	rc = pthread_cond_init(&oven_cond, NULL);
	if(rc != 0) {
 		printf("ERROR: return code from pthread_cond_init() is %d\n", rc) ;
       		exit(-1);
	}
	rc = pthread_cond_init(&packer_cond, NULL);
	if(rc != 0) {
    		printf("ERROR: return code from pthread_cond_init() is %d\n", rc) ;
       		exit(-1);
	}
	rc = pthread_cond_init(&deliverer_cond, NULL);
	if(rc != 0) {
 		printf("ERROR: return code from pthread_cond_init() is %d\n", rc) ;
       		exit(-1);
	}

	pthread_t *threads;
	int *id;

	threads = malloc(numOfClients * sizeof(pthread_t));
	id = malloc(numOfClients * sizeof(int));


	//--------------------------THREAD CREATION------------------------------------
	temp = seed;
	
	for(int i = 0 ; i < numOfClients ; i++){
		id[i] = i + 1;
		rc = pthread_create(&threads[i], NULL, pizzaOrder, &id[i]);
		if(rc != 0){
			printf("ERROR: return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
		//wait for the next order
		sleep(rand_r(&temp) % TORDERHIGH + TORDERLOW);
		temp = seed - id[i];
	}

	//--------------------------THREAD JOIN-----------------------------------------
	for(int i = 0 ; i < numOfClients ; i++){
		rc = pthread_join(threads[i], NULL);
		if(rc != 0){
			printf("ERROR: return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
	}


	//------------------------PRINTING STATISTICS-----------------------------------
	
	//total income
	printf("The total income collected is  %d . \n", total_income);
	// successfull deliveries
	printf("Successfull deliveries   --> %d \n",total_completed_deliveries);
	// unsuccessfull deliveries
	printf("Unsuccessfull deliveries --> %d \n",numOfClients - total_completed_deliveries);
	//waiting time
	average = (double)average_waiting_time / (double)numOfClients;
	printf("The average waiting time was: %.2f minutes \n", average);
	printf("The maximum waiting time was: %d minutes \n", max_waiting_time);
	//service time
	average = (double)average_total_time / (double)total_completed_deliveries;
	printf("The average service time was: %.2f minutes \n", average);
	printf("The maximum service time was: %d minutes \n", max_total_time);
	//cooling time
	average = (double)average_cooling_time / (double)total_completed_deliveries;
	printf("The average cooling time was: %.2f minutes \n", average);
	printf("The maximum cooling time was: %d minutes \n", max_cooling_time);

	//-------------------------MUTEX BEIGN DESTROYED HERE---------------------------
	rc = pthread_mutex_destroy(&callReciever_mtx);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_destroy(&cook_mtx);
	if(rc != 0) {
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_mutex_destroy(&oven_mtx);
	if(rc != 0) {
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_mutex_destroy(&packer_mtx);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_destroy(&deliverer_mtx);
	if(rc != 0) {
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_mutex_destroy(&screen_mtx);
	if(rc != 0) {
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_mutex_destroy(&total_money_mtx);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_destroy(&complete_delivery_mtx);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_destroy(&max_waiting_time_mtx);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_destroy(&max_total_time_mtx);
	if(rc != 0) {
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_mutex_destroy(&max_cooling_time_mtx);
	if(rc != 0) {
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_mutex_destroy(&average_waiting_time_mtx);
	if(rc != 0){
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc) ;
		exit(-1);
	}
	rc = pthread_mutex_destroy(&average_total_time_mtx);
	if(rc != 0) {
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_mutex_destroy(&average_cooling_time_mtx);
	if(rc != 0) {
		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_cond_destroy(&callReciever_cond);
	if(rc != 0) {
    		printf("ERROR: return code from pthread_cond_destroy() is %d\n", rc) ;
       		exit(-1);
	}
 	rc = pthread_cond_destroy(&cook_cond);
	if(rc != 0) {
		printf("ERROR: return code from pthread_cond_destroy() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_cond_destroy(&oven_cond);
	if(rc != 0) {
		printf("ERROR: return code from pthread_cond_destroy() is %d\n", rc);
		exit(-1);
	}
	rc = pthread_cond_destroy(&packer_cond);
	if(rc != 0) {
    		printf("ERROR: return code from pthread_cond_destroy() is %d\n", rc) ;
       		exit(-1);
	}
	rc = pthread_cond_destroy(&deliverer_cond);
	if(rc != 0) {
		printf("ERROR: return code from pthread_cond_destroy() is %d\n", rc);
		exit(-1);
	}

	//free memory that was reserved with malloc
	free(threads);
	free(id);

	//exit program
	exit(-1) ;
}