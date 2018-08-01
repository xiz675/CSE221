//
//  net0.cpp
//  server
//
//  Created by zxw on 2018/6/6.
//  Copyright © 2018 zxw. All rights reserved.
//

#include "rely.h"
#include "TcpUtil.h"
#include <vector>

static void server(const uint32_t)
{
    constexpr uint16_t port = 8998;
    constexpr uint32_t authKey = 0xabadcafe;
    TcpServer server(port);
    while (true)
    {
        TcpConnection conn = server.WaitConnection();
        size_t recvSize = 0, sendSize = 0;
        uint32_t loopCnt = 0;
        uint32_t auth1 = 0, auth2 = 0;
        if (!conn.ReceiveData(&auth1, sizeof(auth1)))
            continue;
        conn.ReceiveData(&recvSize, sizeof(recvSize));
        conn.ReceiveData(&sendSize, sizeof(sendSize));
        conn.ReceiveData(&loopCnt, sizeof(loopCnt));
        conn.ReceiveData(&auth2, sizeof(auth2));
        if (auth1 != authKey || auth2 != authKey)
        {
            logger::Warn("bad request.\n");
            continue;
        }

        std::vector<uint8_t> bufferRecv(recvSize);
        std::vector<uint8_t> bufferSend(sendSize);
        for (uint32_t i = 0; i < loopCnt; i++)
        {
            if (!conn.ReceiveData(bufferRecv.data(), recvSize))
                break;
            if (!conn.SendData(bufferSend.data(), sendSize))
                break;
        }
        logger::Success("test finished.\n");
    }
}


static const uint32_t DummyId = RegistTest("network server", 'Y', false, &server, 1, false);
