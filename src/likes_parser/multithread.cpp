


#include "multithread.h"
#include "using_openssl.h"


using namespace vk_parse;

requests_pool::requests_pool(int threads_num):
done(false)
{

    for (int i{}; i < threads_num; ++i ){

        threads.emplace_back([this](){

            simple_https::https_client httpsClient("api.vk.com");

            while(!this->done){
                try{

                    std::function<void(simple_https::https_client&)> task;

                    if ( this->task_queue.try_pop(task) ){
                        task(httpsClient);

                    } else {
                        std::this_thread::yield();
                    }
                } catch(...){
                    this->exc_ptr = std::current_exception();
                }

            }
            
        });
    }
}

std::future<std::string> requests_pool::submit(std::function<std::string(simple_https::https_client&)> func) {

    auto task = std::make_shared<std::packaged_task<std::string(simple_https::https_client&)>>( std::move(func) );
    std::future<std::string> res = task->get_future();

    task_queue.push([task]( simple_https::https_client &httpsClient ){
        (*task)(httpsClient);
    });
    
    return res;
}

requests_pool::~requests_pool() {
    done = true;
    if (exc_ptr){
        std::rethrow_exception(exc_ptr);
    }
    for ( auto &thread:threads ) thread.join();

}
