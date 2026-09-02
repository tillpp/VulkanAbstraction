#include "Queue.hpp"

Queue::Queue():vk::raii::Queue(nullptr){

}
void Queue::create(DeviceSetup& deviceSetup){
    deviceSetup.queues.push_back(this);
}
