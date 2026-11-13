#include "MemorySession.h"

using namespace ReservationDomain;

namespace ReservationInfrastructure
{
    std::optional<ReservationRequest> MemorySession::findById(const RequestId &requestId)
    {
        const std::string idStr = requestId.getValue();

        auto stagedIt = stagedChanges_.find(idStr);
        if (stagedIt != stagedChanges_.end())
        {
            const auto &s = stagedIt->second.state;
            return ReservationRequest::restore(
                RequestId(s.requestId),
                ProductId(s.productId),
                DepartmentId(s.departmentId),
                Quantity(s.requestedQuantity),
                s.status,
                s.rejectionReason
            );
        }

        auto it = storage_->find(idStr);
        if (it == storage_->end())
        {
            return std::nullopt;
        }

        const auto &s = it->second;

        loadedRevisions_[idStr] = s.revision;

        return ReservationRequest::restore(
            RequestId(s.requestId),
            ProductId(s.productId),
            DepartmentId(s.departmentId),
            Quantity(s.requestedQuantity),
            s.status,
            s.rejectionReason
        );
    }

    void MemorySession::save(const ReservationRequest &request)
    {
        const std::string idStr = request.getRequestId().getValue();

        bool isNew = false;
        long expectedRevision = 0;

        auto loadedIt = loadedRevisions_.find(idStr);
        if (loadedIt != loadedRevisions_.end())
        {
            expectedRevision = loadedIt->second;
            isNew = false;
        }
        else if (storage_->find(idStr) == storage_->end())
        {
            isNew = true;
            expectedRevision = 0;
        }
        else
        {
            isNew = false;
            expectedRevision = 0;
        }

        StoredReservationState stateToStage{
            request.getRequestId().getValue(),
            request.getProductId().getValue(),
            request.getDepartmentId().getValue(),
            request.getRequestedQuantity().getValue(),
            request.getStatus(),
            request.getRejectionReason(),
            isNew ? 1 : expectedRevision + 1
        };

        stagedChanges_[idStr] = StagedChange{ stateToStage, expectedRevision, isNew };
    }

    void MemorySession::commit()
    {
        for (const auto &[id, staged] : stagedChanges_)
        {
            auto it = storage_->find(id);
            if (staged.isNew)
            {
                if (it != storage_->end())
                {
                    std::string errorMsg = "Конфлікт версій: заявка з ID " + id + " вже існує у сховищі.";
                    rollback();
                    throw std::runtime_error(errorMsg);
                }
            }
            else
            {
                if (it == storage_->end())
                {
                    std::string errorMsg = "Помилка Unit of Work: заявку " + id + " не знайдено у сховищі.";
                    rollback();
                    throw std::runtime_error(errorMsg);
                }
                if (it->second.revision != staged.expectedRevision)
                {
                    std::string errorMsg = "Конфлікт версій для " + id + ": очікувалася ревізія " +
                        std::to_string(staged.expectedRevision) + ", але у сховищі ревізія " +
                        std::to_string(it->second.revision);
                    rollback();
                    throw std::runtime_error(errorMsg);
                }
            }
        }

        for (const auto &[id, staged] : stagedChanges_)
        {
            (*storage_)[id] = staged.state;
            loadedRevisions_[id] = staged.state.revision;
        }

        stagedChanges_.clear();
    }

    void MemorySession::rollback()
    {
        stagedChanges_.clear();
    }

    void MemorySession::seed(const StoredReservationState &state)
    {
        (*storage_)[state.requestId] = state;
    }

    long MemorySession::getRevision(const RequestId &requestId) const
    {
        auto it = storage_->find(requestId.getValue());
        if (it == storage_->end())
        {
            return 0;
        }
        return it->second.revision;
    }
}