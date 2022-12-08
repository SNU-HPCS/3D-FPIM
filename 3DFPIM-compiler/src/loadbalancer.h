#include <iostream>
#include <string>

#include "common.h"

class LoadBalancer {

    private:

        ModelImpl* model_;

    public:

        LoadBalancer(ModelImpl* model);

        void buildNetwork();
};
