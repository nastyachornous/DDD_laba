#ifndef MEMORY_SESSION_H
#define MEMORY_SESSION_H

#include "../../Model/ReservationRequest.h"
#include "../../Application/Ports/IReservationRepository.h"
#include "../../Application/Ports/IUnitOfWork.h"
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>
#include <stdexcept>

namespace ReservationInfrastructure
{
    struct StoredReservationState
    {
        std::string requestId;
        std::string productId;
        std::string departmentId;
        int requestedQuantity;
        ReservationDomain::ReservationStatus status;
        std::string rejectionReason;
        long revision;
    };

    using MemoryStorage = std::unordered_map<std::string, StoredReservationState>;

    struct StagedChange
    {
        StoredReservationState state;
        long expectedRevision;
        bool isNew;
    };

    class MemorySession : public ReservationApplication::IReservationRepository,
                          public ReservationApplication::IUnitOfWork
    {
    private:
        std::shared_ptr<MemoryStorage> storage_;
        std::unordered_map<std::string, StagedChange> stagedChanges_;
        std::unordered_map<std::string, long> loadedRevisions_;

    public:
        MemorySession() : storage_(std::make_shared<MemoryStorage>()) {}
        explicit MemorySession(std::shared_ptr<MemoryStorage> sharedStorage)
            : storage_(std::move(sharedStorage)) {}

        std::optional<ReservationDomain::ReservationRequest>
        findById(const ReservationDomain::RequestId &requestId) override;

        void save(const ReservationDomain::ReservationRequest &request) override;

        void commit() override;
        void rollback() override;

        void seed(const StoredReservationState &state);
        long getRevision(const ReservationDomain::RequestId &requestId) const;
        std::shared_ptr<MemoryStorage> getStorage() const { return storage_; }
    };
}

#endif 