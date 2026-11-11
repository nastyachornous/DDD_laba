
#ifndef I_STOCK_AVAILABILITY_PORT_H
#define I_STOCK_AVAILABILITY_PORT_H

#include "../../Model/ValueObjects.h"

namespace ReservationApplication
{

    class IStockAvailabilityPort
    {
    public:
        virtual ~IStockAvailabilityPort() = default;

        virtual ReservationDomain::AvailableQuantity getAvailableQuantity(
            const ReservationDomain::ProductId &productId) = 0;
    };

}

#endif
