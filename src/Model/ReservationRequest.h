
#ifndef RESERVATION_REQUEST_H
#define RESERVATION_REQUEST_H

#include "ValueObjects.h"
#include <optional>
#include <string>

namespace ReservationDomain
{

    enum class ReservationStatus
    {
        Submitted,
        Reserved,
        Rejected
    };

    struct Reservation
    {
        RequestId requestId;
        ProductId productId;
        DepartmentId departmentId;
        Quantity quantity;
    };

    class ReservationRequest
    {
    private:
        RequestId requestId;
        ProductId productId;
        DepartmentId departmentId;
        Quantity requestedQuantity;

        ReservationStatus status;
        std::string rejectionReason;

    public:
        ReservationRequest(
            const RequestId &requestId,
            const ProductId &productId,
            const DepartmentId &departmentId,
            const Quantity &requestedQuantity);

        // Оновлення лабораторної №4
        static ReservationRequest restore(
            const RequestId &requestId,
            const ProductId &productId,
            const DepartmentId &departmentId,
            const Quantity &requestedQuantity,
            ReservationStatus status,
            const std::string &rejectionReason);

        std::optional<Reservation> process(
            const AvailableQuantity &availableQuantity);

        ReservationStatus getStatus() const;

        const std::string &getRejectionReason() const;

        const RequestId &getRequestId() const;

        const ProductId &getProductId() const;

        const DepartmentId &getDepartmentId() const;

        const Quantity &getRequestedQuantity() const;
    };

}

#endif
