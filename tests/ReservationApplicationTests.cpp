
#include "../src/Application/Services/ReservationService.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <optional>

using namespace ReservationDomain;
using namespace ReservationApplication;

// Простий репозиторій для тестування
class FakeReservationRepository : public IReservationRepository
{
public:
    std::optional<ReservationRequest> storedRequest;
    bool saveCalled = false;

    std::optional<ReservationRequest> findById(
        const RequestId &requestId) override
    {
        if (!storedRequest.has_value())
            return std::nullopt;

        if (storedRequest->getId() != requestId)
            return std::nullopt;

        return storedRequest;
    }

    void save(const ReservationRequest &request) override
    {
        storedRequest = request;
        saveCalled = true;
    }
};

// Простий Unit of Work для тестування
class FakeUnitOfWork : public IUnitOfWork
{
public:
    bool commitCalled = false;
    bool rollbackCalled = false;
    bool failCommit = false;

    void commit() override
    {
        commitCalled = true;

        if (failCommit)
            throw std::runtime_error("Commit failed");
    }

    void rollback() override
    {
        rollbackCalled = true;
    }
};

// Імітація сервісу перевірки залишків
class FakeStockAvailability : public IStockAvailabilityPort
{
public:
    int availableQuantity = 10;
    bool fail = false;

    AvailableQuantity getAvailableQuantity(
        const ProductId &productId) override
    {
        if (fail)
            throw std::runtime_error("Stock service unavailable");

        return AvailableQuantity(availableQuantity);
    }
};

// Перевірка результату тесту
void check(bool condition, const std::string &testName)
{
    if (!condition)
        throw std::runtime_error("FAILED: " + testName);

    std::cout << "PASSED: " << testName << '\n';
}

// Створення тестової заявки
ReservationRequest createRequest()
{
    return ReservationRequest(
        RequestId("REQ-001"),
        ProductId("PROD-001"),
        DepartmentId("DEPT-001"),
        Quantity(5)
    );
}

int main()
{
    try
    {
        // 1. Успішне резервування
        {
            FakeReservationRepository repository;
            FakeUnitOfWork unitOfWork;
            FakeStockAvailability stock;

            repository.storedRequest = createRequest();

            ReservationService service(repository, unitOfWork, stock);

            auto result = service.processRequest(RequestId("REQ-001"));

            check(
                result.status == ReservationStatus::Reserved &&
                result.reservation.has_value() &&
                repository.saveCalled &&
                unitOfWork.commitCalled,
                "Successful reservation"
            );
        }

        // 2. Недостатня кількість товару
        {
            FakeReservationRepository repository;
            FakeUnitOfWork unitOfWork;
            FakeStockAvailability stock;

            stock.availableQuantity = 2;
            repository.storedRequest = createRequest();

            ReservationService service(repository, unitOfWork, stock);

            auto result = service.processRequest(RequestId("REQ-001"));

            check(
                result.status == ReservationStatus::Rejected &&
                !result.reservation.has_value() &&
                !result.rejectionReason.empty() &&
                repository.saveCalled &&
                unitOfWork.commitCalled,
                "Insufficient stock"
            );
        }

        // 3. Заявку не знайдено
        {
            FakeReservationRepository repository;
            FakeUnitOfWork unitOfWork;
            FakeStockAvailability stock;

            ReservationService service(repository, unitOfWork, stock);

            bool exceptionThrown = false;

            try
            {
                service.processRequest(RequestId("REQ-404"));
            }
            catch (const std::runtime_error &)
            {
                exceptionThrown = true;
            }

            check(
                exceptionThrown &&
                !unitOfWork.commitCalled &&
                !repository.saveCalled,
                "Request not found"
            );
        }

        // 4. Технічна помилка отримання залишку
        {
            FakeReservationRepository repository;
            FakeUnitOfWork unitOfWork;
            FakeStockAvailability stock;

            repository.storedRequest = createRequest();
            stock.fail = true;

            ReservationService service(repository, unitOfWork, stock);

            bool exceptionThrown = false;

            try
            {
                service.processRequest(RequestId("REQ-001"));
            }
            catch (const std::runtime_error &)
            {
                exceptionThrown = true;
            }

            check(
                exceptionThrown &&
                unitOfWork.rollbackCalled &&
                !unitOfWork.commitCalled,
                "Stock service failure"
            );
        }

        // 5. Помилка commit
        {
            FakeReservationRepository repository;
            FakeUnitOfWork unitOfWork;
            FakeStockAvailability stock;

            repository.storedRequest = createRequest();
            unitOfWork.failCommit = true;

            ReservationService service(repository, unitOfWork, stock);

            bool exceptionThrown = false;

            try
            {
                service.processRequest(RequestId("REQ-001"));
            }
            catch (const std::runtime_error &)
            {
                exceptionThrown = true;
            }

            check(
                exceptionThrown &&
                unitOfWork.commitCalled &&
                unitOfWork.rollbackCalled,
                "Commit failure"
            );
        }

        std::cout << "\nAll Application tests passed!\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
