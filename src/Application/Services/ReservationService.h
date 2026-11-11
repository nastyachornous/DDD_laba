
#ifndef RESERVATION_SERVICE_H
#define RESERVATION_SERVICE_H

#include "../Ports/IReservationRepository.h"
#include "../Ports/IUnitOfWork.h"
#include "../Ports/IStockAvailabilityPort.h"

#include <optional>
#include <string>

namespace ReservationApplication
{

    struct ReservationProcessingResult
    {
        ReservationDomain::ReservationStatus status;
        std::optional<ReservationDomain::Reservation> reservation;
        std::string rejectionReason;
    };

    class ReservationService
    {
    private:
        IReservationRepository &repository;
        IUnitOfWork &unitOfWork;
        IStockAvailabilityPort &stockAvailability;

    public:
        ReservationService(
            IReservationRepository &repository,
            IUnitOfWork &unitOfWork,
            IStockAvailabilityPort &stockAvailability);

        ReservationProcessingResult processRequest(
            const ReservationDomain::RequestId &requestId);
    };

}

#endif
