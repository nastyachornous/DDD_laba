
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

    // Оновлення лабораторної №4
    ReservationRequest ReservationRequest::restore(
        const RequestId &requestId,
        const ProductId &productId,
        const DepartmentId &departmentId,
        const Quantity &requestedQuantity,
        ReservationStatus status,
        const std::string &rejectionReason)
    {
        ReservationRequest request(
            requestId,
            productId,
            departmentId,
            requestedQuantity);

        switch (status)
        {
        case ReservationStatus::Submitted:
        case ReservationStatus::Reserved:
            if (!rejectionReason.empty())
            {
                throw std::invalid_argument(
                    "Submitted or Reserved request cannot have a rejection reason");
            }
            break;

        case ReservationStatus::Rejected:
            if (isBlank(rejectionReason))
            {
                throw std::invalid_argument(
                    "Rejected request must have a rejection reason");
            }
            break;

        default:
            throw std::invalid_argument(
                "Unknown reservation status");
        }

        request.status = status;
        request.rejectionReason = rejectionReason;

        return request;
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
            "Недостатня кількість товару. Запитано: " +
            std::to_string(requestedQuantity.getValue()) +
            ", Доступно: " +
            std::to_string(availableQuantity.getValue());

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
