#if !defined(RESIP_SIPMESSAGE_HXX)
#define RESIP_SIPMESSAGE_HXX 

#include "resiprocate/os/Socket.hxx"

#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <list>
#include <vector>
#include <utility>

#include "resiprocate/Headers.hxx"
#include "resiprocate/Message.hxx"
#include "resiprocate/ParserCategories.hxx"
#include "resiprocate/ParserContainer.hxx"
#include "resiprocate/Transport.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/Contents.hxx"
#include "resiprocate/os/Data.hxx"

#if !defined(WIN32)
#include <pthread.h>
#endif 

namespace resip
{

class Contents;
class UnknownHeaderType;

class SipMessage : public Message
{
   public:
      typedef std::list< std::pair<Data, HeaderFieldValueList*> > UnknownHeaders;

      explicit SipMessage(const Transport* fromWire = 0);
      SipMessage(const SipMessage& message);
      
      // returns the transaction id from the branch or if 2543, the computed hash
      virtual const Data& getTransactionId() const;

      const Data& getRFC2543TransactionId() const;
      void setRFC2543TransactionId(const Data& tid);
      
      virtual ~SipMessage();

      static SipMessage* make(const Data& buffer, bool isExternal = false);

      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) {}

            const char* name() const { return "SipMessage::Exception"; }
      };

      void setFromTU() 
      {
         mIsExternal = false;
      }

      void setFromExternal()
      {
         mIsExternal = true;
      }
      
      bool isExternal() const
      {
         return mIsExternal;
      }

      virtual bool isClientTransaction() const;
      
      virtual std::ostream& encode(std::ostream& str) const;
      std::ostream& encodeEmbedded(std::ostream& str) const;
      
      Data brief() const;

      bool isRequest() const;
      bool isResponse() const;

      const RequestLine& 
      header(const RequestLineType& l) const;

      RequestLine& 
      header(const RequestLineType& l);

      const StatusLine& 
      header(const StatusLineType& l) const;

      StatusLine& 
      header(const StatusLineType& l);

      bool exists(const HeaderBase& headerType) const;
      void remove(const HeaderBase& headerType);

#ifdef PARTIAL_TEMPLATE_SPECIALIZATION

      template <class T>
      typename T::UnknownReturn&
      SipMessage::header(const T& headerType)
      {
         HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), T::Single);
         if (hfvs->getParserContainer() == 0)
         {
            hfvs->setParserContainer(new ParserContainer<typename T::Type>(hfvs, headerType.getTypeNum()));
         }
         return T::knownReturn(hfvs->getParserContainer());
      };

      // looks identical, but it isn't -- ensureHeaders CONST called -- may throw
      template <class T>
      const typename T::UnknownReturn&
      SipMessage::header(const T& headerType) const
      {
         HeaderFieldValueList* hfvs = ensureHeaders(headerType.getTypeNum(), T::Single);
         if (hfvs->getParserContainer() == 0)
         {
            hfvs->setParserContainer(new ParserContainer<typename T::Type>(hfvs, headerType.getTypeNum()));
         }
         return T::knownReturn(hfvs->getParserContainer());
      };

#else

#define defineHeader(_header)                                                           \
      const _header##_Header::Type& header(const _header##_Header& headerType) const;   \
      _header##_Header::Type& header(const _header##_Header& headerType)

#define defineMultiHeader(_header)                                                                                      \
      const ParserContainer<_header##_MultiHeader::Type>& header(const _header##_MultiHeader& headerType) const;        \
      ParserContainer<_header##_MultiHeader::Type>& header(const _header##_MultiHeader& headerType)

      defineHeader(CSeq);
      defineHeader(CallId);
      defineHeader(AuthenticationInfo);
      defineHeader(ContentDisposition);
      defineHeader(ContentTransferEncoding);
      defineHeader(ContentEncoding);
      defineHeader(ContentLength);
      defineHeader(ContentType);
      defineHeader(Date);
      defineHeader(Event);
      defineHeader(Expires);
      defineHeader(From);
      defineHeader(InReplyTo);
      defineHeader(MIMEVersion);
      defineHeader(MaxForwards);
      defineHeader(MinExpires);
      defineHeader(Organization);
      defineHeader(Priority);
      defineHeader(ReferTo);
      defineHeader(ReferredBy);
      defineHeader(Replaces);
      defineHeader(ReplyTo);
      defineHeader(RetryAfter);
      defineHeader(Server);
      defineHeader(Subject);
      defineHeader(Timestamp);
      defineHeader(To);
      defineHeader(UserAgent);
      defineHeader(Warning);

      defineMultiHeader(SecurityClient);
      defineMultiHeader(SecurityServer);
      defineMultiHeader(SecurityVerify);

      defineMultiHeader(Authorization);
      defineMultiHeader(ProxyAuthenticate);
      defineMultiHeader(WWWAuthenticate);
      defineMultiHeader(ProxyAuthorization);

      defineMultiHeader(Accept);
      defineMultiHeader(AcceptEncoding);
      defineMultiHeader(AcceptLanguage);
      defineMultiHeader(AlertInfo);
      defineMultiHeader(Allow);
      defineMultiHeader(AllowEvents);
      defineMultiHeader(CallInfo);
      defineMultiHeader(Contact);
      defineMultiHeader(ContentLanguage);
      defineMultiHeader(ErrorInfo);
      defineMultiHeader(ProxyRequire);
      defineMultiHeader(RecordRoute);
      defineMultiHeader(Require);
      defineMultiHeader(Route);
      defineMultiHeader(SubscriptionState);
      defineMultiHeader(Supported);
      defineMultiHeader(Unsupported);
      defineMultiHeader(Via);

#endif // METHOD_TEMPLATES

      // unknown header interface
      const StringCategories& header(const UnknownHeaderType& symbol) const;
      StringCategories& header(const UnknownHeaderType& symbol);
      bool exists(const UnknownHeaderType& symbol) const;
      void remove(const UnknownHeaderType& symbol);

      // typeless header interface
      const HeaderFieldValueList* getRawHeader(Headers::Type headerType) const;
      void setRawHeader(const HeaderFieldValueList* hfvs, Headers::Type headerType);
      const UnknownHeaders& getRawUnknownHeaders() const {return mUnknownHeaders;}

      Contents* getContents() const;
      // removes the contents from the message
      std::auto_ptr<Contents> releaseContents();

      void setContents(const Contents* contents);
      void setContents(std::auto_ptr<Contents> contents);

      // transport interface
      void setStartLine(const char* start, int len); 

      void setBody(const char* start, int len); 
      
      // add HeaderFieldValue given enum, header name, pointer start, content length
      void addHeader(Headers::Type header,
                     const char* headerName, int headerLen, 
                     const char* start, int len);

      // Interface used to determine which Transport was used to receive a
      // particular SipMessage. If the SipMessage was not received from the
      // wire, getReceivedTransport() returns 0. Set in constructor
      const Transport* getReceivedTransport() const { return mTransport; }

      // Returns the source tuple that the message was received from
      // only makes sense for messages received from the wire
      void setSource(const Tuple& tuple) { mSource = tuple; }
      const Tuple& getSource() const { return mSource; }
      
      // Used by the stateless interface to specify where to send a request/response
      void setDestination(const Tuple& tuple) { mDestination = tuple; }
      Tuple& getDestination() { return mDestination; }

      void addBuffer(char* buf);

      // returns the encoded buffer which was encoded by resolve()
      // should only be called by the TransportSelector
      Data& getEncoded();

      UInt64 getCreatedTimeMicroSec() {return mCreatedTime;}

      // deal with a notion of an "out-of-band" forced target for SIP routing
      void setForceTarget(const Uri& uri);
      void clearForceTarget();
      const Uri& getForceTarget() const;
      bool hasForceTarget() const;

      const Data& getTlsDomain() const { return mTlsDomain; }
      void setTlsDomain(const Data& domain) { mTlsDomain = domain; }
      
   private:
      void compute2543TransactionHash() const;
      
      void copyFrom(const SipMessage& message);

      HeaderFieldValueList* ensureHeaders(Headers::Type type, bool single);
      HeaderFieldValueList* ensureHeaders(Headers::Type type, bool single) const; // throws if not present

      // not available
      SipMessage& operator=(const SipMessage&);

      // indicates this message came from the wire, set by the Transport
      bool mIsExternal;
      
      // raw text corresponding to each typed header (not yet parsed)
      mutable HeaderFieldValueList* mHeaders[Headers::MAX_HEADERS];

      // raw text corresponding to each unknown header
      mutable UnknownHeaders mUnknownHeaders;
  
      // !jf!
      const Transport* mTransport;

      // For messages received from the wire, this indicates where it came
      // from. Can be used to get to the Transport and/or reliable Connection
      Tuple mSource;

      // Used by the TU to specify where a message is to go
      Tuple mDestination;
      
      // Raw buffers coming from the Transport. message manages the memory
      std::vector<char*> mBufferList;

      // special case for the first line of message
      mutable HeaderFieldValueList* mStartLine;

      // raw text for the contents (all of them)
      mutable HeaderFieldValue* mContentsHfv;

      // lazy parser for the contents
      mutable Contents* mContents;

      // cached value of a hash of the transaction id for a message received
      // from a 2543 sip element. as per rfc3261 see 17.2.3
      mutable Data mRFC2543TransactionId;

      // is a request or response
      mutable bool mRequest;
      mutable bool mResponse;

      Data mEncoded; // to be retransmitted
      UInt64 mCreatedTime;

      // used when next element is a strict router OR 
      // client forces next hop OOB
      Uri* mForceTarget;

      // domain associated with this message for tls cert
      Data mTlsDomain;

      friend class TransportSelector;
};

}

#undef ensureHeaderTypeUseable
#undef ensureSingleHeader
#undef ensureMultiHeader

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */