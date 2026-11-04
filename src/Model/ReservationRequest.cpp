#include "ReservationRequest.h"
#include <stdexcept>

namespace ReservationDomain
{

    ReservationRequest::ReservationRequest(
        const RequestId &requestId,
        const ProductId &productId,
        const DepartmentId &departmentId,
        const Quantity &requestedQuantity)
        : requestId(requestId),
          productId(productId),
          departmentId(departmentId),
          requestedQuantity(requestedQuantity),
          status(ReservationStatus::Submitted),
          rejectionReason("")
    {
    }

    std::optional<Reservation> ReservationRequest::process(
        const AvailableQuantity &availableQuantity)
    {
        if (status != ReservationStatus::Submitted)
        {
            throw std::logic_error(
                "Only submitted requests can be processed");
        }

        if (requestedQuantity.getValue() <=
            availableQuantity.getValue())
        {

            status = ReservationStatus::Reserved;
            rejectionReason = "";

            return Reservation{
                requestId,
                productId,
                departmentId,
                requestedQuantity};
        }

        status = ReservationStatus::Rejected;

        rejectionReason =
            "Insufficient available quantity";

        return std::nullopt;
    }

    ReservationStatus ReservationRequest::getStatus() const
    {
        return status;
    }

    const std::string &ReservationRequest::getRejectionReason() const
    {
        return rejectionReason;
    }

    const RequestId &ReservationRequest::getRequestId() const
    {
        return requestId;
    }

    const ProductId &ReservationRequest::getProductId() const
    {
        return productId;
    }

    const DepartmentId &ReservationRequest::getDepartmentId() const
    {
        return departmentId;
    }

    const Quantity &ReservationRequest::getRequestedQuantity() const
    {
        return requestedQuantity;
    }

}