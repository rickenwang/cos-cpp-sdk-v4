#include <string.h>
#include <stdint.h>
#include <map>
#include <string>
#include "op/FileOp.h"
#include "Auth.h"

using std::string;
using std::map;
namespace qcloud_cos{

FileUploadTask::FileUploadTask(const string& url, const string& session,
        const string& sign, const string& sha) : m_url(url), m_session(session), m_sign(sign), m_sha(sha)
{
}

FileUploadTask::FileUploadTask(const string& url, const string& session,
        const string& sign, const string& sha, uint64_t offset,
        unsigned char* pbuf, const size_t datalen) : m_url(url), m_session(session), m_sign(sign), m_sha(sha), m_offset(offset), m_pdatabuf(pbuf), m_datalen(datalen), m_resp(""), m_task_success(false)
{
    SDK_LOG_DBG("constructor");
}

void FileUploadTask::run()
{
    m_resp = "";
    m_task_success = false;
    uploadTask();
}

void FileUploadTask::setOffset(uint64_t offset)
{
    m_offset = offset;
}

void FileUploadTask::setBufdata(unsigned char* pbuf, size_t datalen)
{
    m_pdatabuf = pbuf;
    m_datalen  = datalen;
}

bool FileUploadTask::taskSuccess() {
    return m_task_success;
}

string FileUploadTask::getTaskResp() {
    return m_resp;
}


void FileUploadTask::uploadTask()
{
    int loop = 0;
    std::map<string,string> user_params;
    std::map<string,string> user_headers;
    user_headers[HTTP_HEADER_AUTHORIZATION] = m_sign;
    user_params[PARA_OP] = OP_UPLOAD_SLICE_DATA;
    user_params[PARA_SESSION] = m_session;
    user_params[PARA_OFFSET] = StringUtil::Uint64ToString(m_offset);
    if (CosSysConfig::isTakeSha()){
        user_params[PARA_SHA] = m_sha;
    }

    do {
        loop++;
        m_resp = HttpSender::SendSingleFilePostRequest(m_url,user_headers,
            user_params, (const unsigned char*)m_pdatabuf, m_datalen);

        Json::Value dataRspJson = StringUtil::StringToJson(m_resp);
        if (dataRspJson["code"].asInt() == 0){
            m_task_success = true;
#if __WORDSIZE == 32
            SDK_LOG_DBG("file upload task success,offset=%llu, len=%lu, m_task_success: %d", m_offset, m_datalen, m_task_success);
#else
            SDK_LOG_DBG("file upload task success,offset=%lu, len=%lu, m_task_success: %d", m_offset, m_datalen, m_task_success);
#endif
            break;
        } else {
            m_task_success = false;
#if __WORDSIZE == 32
            SDK_LOG_ERR("file upload task fail, offset=%llu, len=%lu, retry_index: %d,  response: %s",m_offset, m_datalen, loop, m_resp.c_str());
#else
            SDK_LOG_ERR("file upload task fail, offset=%lu, len=%lu, retry_index: %d,  response: %s",m_offset, m_datalen, loop, m_resp.c_str());
#endif
        }
    } while (loop <= 1);//kMaxRetryTimes);

    return;
}

}

