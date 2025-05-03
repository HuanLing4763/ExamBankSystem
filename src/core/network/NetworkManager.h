#pragma once
#include <QNetworkAccessManager>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    static QNetworkAccessManager& instance();

private:
    NetworkManager() = default;
    ~NetworkManager() = default;
};