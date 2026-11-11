
#include "ReservationService.h"

#include <stdexcept>

namespace ReservationApplication
{

    ReservationService::ReservationService(
        IReservationRepository &repository,
        IUnitOfWork &unitOfWork,
        IStockAvailabilityPort &stockAvailability)
        : repository(repository),
          unitOfWork(unitOfWork),
          stockAvailability(stockAvailability)
    {
    }

    ReservationProcessingResult ReservationService::processRequest(
        const ReservationDomain::RequestId &requestId)
    {
        auto request = repository.findById(requestId);

        if (!request.has_value())
        {
            throw std::runtime_error("Reservation request not found");
        }

        try
        {
            auto availableQuantity =
                stockAvailability.getAvailableQuantity(request->getProductId());

            auto reservation = request->process(availableQuantity);

            repository.save(*request);

            unitOfWork.commit();

            return {
                request->getStatus(),
                reservation,
                request->getRejectionReason()
            };
        }
        catch (...)
        {
            unitOfWork.rollback();
            throw;
        }
    }

}
