
#ifndef I_RESERVATION_REPOSITORY_H
#define I_RESERVATION_REPOSITORY_H

#include "../../Model/ReservationRequest.h"
#include <optional>

namespace ReservationApplication
{

    class IReservationRepository
    {
    public:
        virtual ~IReservationRepository() = default;

        virtual std::optional<ReservationDomain::ReservationRequest>
        findById(const ReservationDomain::RequestId &requestId) = 0;

        virtual void save(
            const ReservationDomain::ReservationRequest &request) = 0;
    };

}

#endif
